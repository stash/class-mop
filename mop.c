#include "mop.h"

void
mop_call_xs (pTHX_ XSPROTO(subaddr), CV *cv, SV **mark)
{
    dSP;
    PUSHMARK(mark);
    (*subaddr)(aTHX_ cv);
    PUTBACK;
}

#if PERL_VERSION >= 10
UV
mop_check_package_cache_flag (pTHX_ HV *stash)
{
    assert(SvTYPE(stash) == SVt_PVHV);

    /* here we're trying to implement a c version of mro::get_pkg_gen($stash),
     * however the perl core doesn't make it easy for us. It doesn't provide an
     * api that just does what we want.
     *
     * However, we know that the information we want is, inside the core,
     * available using HvMROMETA(stash)->pkg_gen. Unfortunately, although the
     * HvMROMETA macro is public, it is implemented using Perl_mro_meta_init,
     * which is not public and only available inside the core, as the mro
     * interface as well as the structure returned by mro_meta_init isn't
     * considered to be stable yet.
     *
     * Perl_mro_meta_init isn't declared static, so we could just define it
     * ourselfs if perls headers don't do that for us, except that won't work
     * on platforms where symbols need to be explicitly exported when linking
     * shared libraries.
     *
     * So our, hopefully temporary, solution is to be even more evil and
     * basically reimplement HvMROMETA in a very fragile way that'll blow up
     * when the relevant parts of the mro implementation in core change.
     *
     * :-(
     *
     */

    return HvAUX(stash)->xhv_mro_meta
         ? HvAUX(stash)->xhv_mro_meta->pkg_gen
         : 0;
}

#else /* pre 5.10.0 */

UV
mop_check_package_cache_flag (pTHX_ HV *stash)
{
    PERL_UNUSED_ARG(stash);
    assert(SvTYPE(stash) == SVt_PVHV);

    return PL_sub_generation;
}
#endif

SV *
mop_call0 (pTHX_ SV *const self, SV *const method)
{
    dSP;
    SV *ret;

    PUSHMARK(SP);
    XPUSHs(self);
    PUTBACK;

    call_sv(method, G_SCALAR | G_METHOD);

    SPAGAIN;
    ret = POPs;
    PUTBACK;

    return ret;
}

int
mop_get_code_info (SV *coderef, char **pkg, char **name)
{
    if (!SvOK(coderef) || !SvROK(coderef) || SvTYPE(SvRV(coderef)) != SVt_PVCV) {
        return 0;
    }

    coderef = SvRV(coderef);

    /* sub is still being compiled */
    if (!CvGV(coderef)) {
        return 0;
    }

    /* I think this only gets triggered with a mangled coderef, but if
       we hit it without the guard, we segfault. The slightly odd return
       value strikes me as an improvement (mst)
    */
#ifdef isGV_with_GP
    if ( isGV_with_GP(CvGV(coderef)) ) {
#endif
        GV *gv   = CvGV(coderef);
        *pkg     = HvNAME( GvSTASH(gv) ? GvSTASH(gv) : CvSTASH(coderef) );
        *name    = GvNAME( CvGV(coderef) );
#ifdef isGV_with_GP
    } else {
        *pkg     = "__UNKNOWN__";
        *name    = "__ANON__";
    }
#endif

    return 1;
}

void
mop_get_package_symbols (HV *stash, type_filter_t filter, get_package_symbols_cb_t cb, void *ud)
{
    HE *he;

    (void)hv_iterinit(stash);

    if (filter == TYPE_FILTER_NONE) {
        while ( (he = hv_iternext(stash)) ) {
            STRLEN keylen;
            const char *key = HePV(he, keylen);
            if (!cb(key, keylen, HeVAL(he), ud)) {
                return;
            }
        }
        return;
    }

    while ( (he = hv_iternext(stash)) ) {
        SV *const gv = HeVAL(he);
        SV *sv = NULL;
        char *key;
        STRLEN keylen;
        char *package;
        SV *fq;

        switch( SvTYPE(gv) ) {
#ifndef SVt_RV
            case SVt_RV:
#endif
            case SVt_PV:
            case SVt_IV:
                /* expand the gv into a real typeglob if it
                 * contains stub functions and we were asked to
                 * return CODE symbols */
                if (filter == TYPE_FILTER_CODE) {
                    if (SvROK(gv)) {
                        /* we don't really care about the length,
                           but that's the API */
                        key = HePV(he, keylen);
                        package = HvNAME(stash);
                        fq = sv_2mortal(newSVpvf("%s::%s", package, key));
                        sv = (SV *)get_cv(SvPV_nolen(fq), 0);
                        break;
                    }

                    key = HePV(he, keylen);
                    gv_init((GV *)gv, stash, key, keylen, GV_ADDMULTI);
                }
                /* fall through */
            case SVt_PVGV:
                switch (filter) {
                    case TYPE_FILTER_CODE:   sv = (SV *)GvCVu(gv); break;
                    case TYPE_FILTER_ARRAY:  sv = (SV *)GvAV(gv);  break;
                    case TYPE_FILTER_IO:     sv = (SV *)GvIO(gv);  break;
                    case TYPE_FILTER_HASH:   sv = (SV *)GvHV(gv);  break;
                    case TYPE_FILTER_SCALAR: sv = (SV *)GvSV(gv);  break;
                    default:
                        croak("Unknown type");
                }
                break;
            default:
                continue;
        }

        if (sv) {
            const char *key = HePV(he, keylen);
            if (!cb(key, keylen, sv, ud)) {
                return;
            }
        }
    }
}

static bool
collect_all_symbols (const char *key, STRLEN keylen, SV *val, void *ud)
{
    HV *hash = (HV *)ud;

    if (!hv_store (hash, key, keylen, newRV_inc(val), 0)) {
        croak("failed to store symbol ref");
    }

    return TRUE;
}

HV *
mop_get_all_package_symbols (HV *stash, type_filter_t filter)
{
    HV *ret = newHV ();
    mop_get_package_symbols (stash, filter, collect_all_symbols, ret);
    return ret;
}

#define DECLARE_KEY(name)                    { #name, #name, NULL, 0 }
#define DECLARE_KEY_WITH_VALUE(name, value)  { #name, value, NULL, 0 }

/* the order of these has to match with those in mop.h */
static struct {
    const char *name;
    const char *value;
    SV *key;
    U32 hash;
} prehashed_keys[key_last] = {
    DECLARE_KEY(name),
    DECLARE_KEY(package),
    DECLARE_KEY(package_name),
    DECLARE_KEY(body),
    DECLARE_KEY_WITH_VALUE(package_cache_flag, "_package_cache_flag"),
    DECLARE_KEY(methods),
    DECLARE_KEY(VERSION),
    DECLARE_KEY(ISA)
};

SV *
mop_prehashed_key_for (mop_prehashed_key_t key)
{
    return prehashed_keys[key].key;
}

U32
mop_prehashed_hash_for (mop_prehashed_key_t key)
{
    return prehashed_keys[key].hash;
}

void
mop_prehash_keys ()
{
    int i;
    for (i = 0; i < key_last; i++) {
        const char *value = prehashed_keys[i].value;
        prehashed_keys[i].key = newSVpv(value, strlen(value));
        PERL_HASH(prehashed_keys[i].hash, value, strlen(value));
    }
}

XS(mop_xs_simple_reader)
{
#ifdef dVAR
    dVAR; dXSARGS;
#else
    dXSARGS;
#endif
    register HE *he;
    mop_prehashed_key_t key = (mop_prehashed_key_t)CvXSUBANY(cv).any_i32;
    SV *self;

    if (items != 1) {
        croak("expected exactly one argument");
    }

    self = ST(0);

    if (!SvROK(self)) {
        croak("can't call %s as a class method", prehashed_keys[key].name);
    }

    if (SvTYPE(SvRV(self)) != SVt_PVHV) {
        croak("object is not a hashref");
    }

    if ((he = hv_fetch_ent((HV *)SvRV(self), prehashed_keys[key].key, 0, prehashed_keys[key].hash))) {
        ST(0) = HeVAL(he);
    }
    else {
        ST(0) = &PL_sv_undef;
    }

    XSRETURN(1);
}

