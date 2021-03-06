#include "mmap_gghlite.h"

#include <gghlite.h>
#include <gghlite/gghlite-defs.h>

#define debug_printf printf
#define CHECK(x, y) if((x) < (y)) { debug_printf(                       \
            "ERROR: fscanf() error encountered when trying to read from file\n" \
            ); }

static void fread_gghlite_params(FILE *fp, gghlite_params_t params);
static void fwrite_gghlite_params(FILE *fp, const gghlite_params_t params);
static void fread_gghlite_sk(FILE *fp, gghlite_sk_t self);
static void fwrite_gghlite_sk(FILE *fp, const gghlite_sk_t self);

/* functions dealing with file reading and writing for encodings */
#define gghlite_enc_fprint fmpz_mod_poly_fprint
#define gghlite_enc_fread fmpz_mod_poly_fread
#define gghlite_enc_fprint_raw fmpz_mod_poly_fprint_raw
#define gghlite_enc_fread_raw fmpz_mod_poly_fread_raw
static int fmpz_mod_poly_fprint_raw(FILE * file, const fmpz_mod_poly_t poly);
static int gghlite_enc_fread_raw(FILE * f, gghlite_enc_t poly);
static int fmpz_poly_fprint_raw(FILE * file, const fmpz_poly_t poly);
static int fmpz_poly_fread_raw(FILE * file, fmpz_poly_t poly);

static void gghlite_params_clear_read_wrapper(const mmap_pp pp)
{
    gghlite_params_clear(pp);
}
static void fread_gghlite_params_wrapper(const mmap_pp pp, FILE *const fp)
{
    fread_gghlite_params(fp, pp);
}
static void fwrite_gghlite_params_wrapper(const mmap_ro_pp pp, FILE *const fp)
{
    fwrite_gghlite_params(fp, pp);
}

static const mmap_pp_vtable gghlite_pp_vtable =
{ .clear = gghlite_params_clear_read_wrapper
  , .fread = fread_gghlite_params_wrapper
  , .fwrite = fwrite_gghlite_params_wrapper
  , .size = sizeof(gghlite_params_t)
};

static int
gghlite_jigsaw_init_gamma_wrapper(const mmap_sk sk, size_t lambda, size_t kappa,
                                  size_t gamma, int *pows, size_t nslots,
                                  unsigned long ncores, aes_randstate_t randstate,
                                  bool verbose)
{
    (void) pows, (void) ncores;
    gghlite_flag_t flags;

    if (nslots > 1) {
        fprintf(stderr, "Error: gghlite only supports a single slot\n");
        return MMAP_ERR;
    }
    flags = GGHLITE_FLAGS_GOOD_G_INV;
    if (verbose)
        flags |= GGHLITE_FLAGS_VERBOSE;
    else
        flags |= GGHLITE_FLAGS_QUIET;
    gghlite_jigsaw_init_gamma(sk, lambda, kappa, gamma, flags,
                              randstate);
    return MMAP_OK;
}

static void gghlite_sk_clear_wrapper(const mmap_sk sk)
{ gghlite_sk_clear(sk, 1); }
static void fread_gghlite_sk_wrapper(const mmap_sk sk, FILE *const fp)
{ fread_gghlite_sk(fp, sk); }
static void fwrite_gghlite_sk_wrapper(const mmap_ro_sk sk, FILE *const fp)
{ fwrite_gghlite_sk(fp, sk); }
static mmap_ro_pp gghlite_sk_to_pp(const mmap_ro_sk sk)
{ return ((const struct _gghlite_sk_struct *const)sk)->params; }

static fmpz_t * fmpz_poly_oz_ideal_norm_wrapper(const mmap_ro_sk sk)
{
    fmpz_t *moduli;

    moduli = calloc(1, sizeof(fmpz_t));
    fmpz_init(moduli[0]);
    fmpz_poly_oz_ideal_norm(moduli[0], ((const struct _gghlite_sk_struct *const)sk)->g, ((const struct _gghlite_sk_struct *const)sk)->params->n, 0);
    return moduli;
}

static size_t gghlite_nslots(const mmap_ro_sk sk __attribute__ ((unused)))
{
    return 1;
}

static size_t gghlite_nzs(const mmap_ro_sk sk)
{
    return ((const struct _gghlite_sk_struct *const)sk)->params->gamma;
}

static const mmap_sk_vtable gghlite_sk_vtable =
{ .init = gghlite_jigsaw_init_gamma_wrapper
  , .clear = gghlite_sk_clear_wrapper
  , .fread = fread_gghlite_sk_wrapper
  , .fwrite = fwrite_gghlite_sk_wrapper
  , .pp = gghlite_sk_to_pp
  , .plaintext_fields = fmpz_poly_oz_ideal_norm_wrapper
  , .nslots = gghlite_nslots
  , .nzs = gghlite_nzs
  , .size = sizeof(gghlite_sk_t)
};

static void gghlite_enc_init_wrapper(const mmap_enc enc, const mmap_ro_pp pp)
{
    gghlite_enc_init(enc, pp);
}
static void gghlite_enc_clear_wrapper(const mmap_enc enc)
{
    gghlite_enc_clear(enc);
}
static void gghlite_enc_fread_raw_wrapper(const mmap_enc enc, FILE *const fp)
{
    gghlite_enc_fread_raw(fp, enc);
}
static void gghlite_enc_fprint_raw_wrapper(const mmap_ro_enc enc, FILE *const fp)
{
    gghlite_enc_fprint_raw(fp, enc);
}
static void gghlite_enc_set_wrapper(const mmap_enc dest, const mmap_ro_enc src)
{
    gghlite_enc_set(dest, src);
}
static void gghlite_enc_add_wrapper(const mmap_enc dest, const mmap_ro_pp pp,
                                    const mmap_ro_enc a, const mmap_ro_enc b)
{
    gghlite_enc_add(dest, pp, a, b);
}
static void gghlite_enc_sub_wrapper(const mmap_enc dest, const mmap_ro_pp pp,
                                    const mmap_ro_enc a, const mmap_ro_enc b)
{
    gghlite_enc_sub(dest, pp, a, b);
}
static void gghlite_enc_mul_wrapper(const mmap_enc dest, const mmap_ro_pp pp,
                                    const mmap_ro_enc a, const mmap_ro_enc b)
{
    gghlite_enc_mul(dest, pp, a, b);
}
static bool gghlite_enc_is_zero_wrapper(const mmap_ro_enc enc, const mmap_ro_pp pp)
{
    return gghlite_enc_is_zero(pp, enc);
}

static void
gghlite_enc_set_gghlite_clr_wrapper(const mmap_enc enc,
                                    const mmap_ro_sk sk, size_t n,
                                    const fmpz_t *plaintext, int *group)
{
    (void) n;
    gghlite_clr_t e;
    gghlite_clr_init(e);
    fmpz_poly_set_coeff_fmpz(e, 0, plaintext[0]);
    gghlite_enc_set_gghlite_clr(enc, sk, e, 1, group, 1);
    gghlite_clr_clear(e);
}

static void fread_gghlite_params(FILE *fp, gghlite_params_t params) {
    int mpfr_base = 10;
    size_t lambda, kappa, gamma, n, ell;
    uint64_t rerand_mask;
    int gghlite_flag_int;
    CHECK(fscanf(fp, "%zu %zu %zu %lu %lu %lu %d\n",
                 &lambda,
                 &gamma,
                 &kappa,
                 &n,
                 &ell,
                 &rerand_mask,
                 &gghlite_flag_int
              ), 7);

    gghlite_params_initzero(params, lambda, kappa, gamma);
    params->n = n;
    params->ell = ell;
    params->rerand_mask = rerand_mask;
    params->flags = gghlite_flag_int;

    fmpz_inp_raw(params->q, fp);
    CHECK(fscanf(fp, "\n"), 0);
    mpfr_inp_str(params->sigma, fp, mpfr_base, MPFR_RNDN);
    CHECK(fscanf(fp, "\n"), 0);
    mpfr_inp_str(params->sigma_p, fp, mpfr_base, MPFR_RNDN);
    CHECK(fscanf(fp, "\n"), 0);
    mpfr_inp_str(params->sigma_s, fp, mpfr_base, MPFR_RNDN);
    CHECK(fscanf(fp, "\n"), 0);
    mpfr_inp_str(params->ell_b, fp, mpfr_base, MPFR_RNDN);
    CHECK(fscanf(fp, "\n"), 0);
    mpfr_inp_str(params->ell_g, fp, mpfr_base, MPFR_RNDN);
    CHECK(fscanf(fp, "\n"), 0);
    mpfr_inp_str(params->xi, fp, mpfr_base, MPFR_RNDN);
    CHECK(fscanf(fp, "\n"), 0);

    gghlite_enc_fread_raw(fp, params->pzt);
    CHECK(fscanf(fp, "\n"), 0);
    CHECK(fscanf(fp, "%zu\n", &params->ntt->n), 1);
    gghlite_enc_fread_raw(fp, params->ntt->w);
    CHECK(fscanf(fp, "\n"), 0);
    gghlite_enc_fread_raw(fp, params->ntt->w_inv);
    CHECK(fscanf(fp, "\n"), 0);
    gghlite_enc_fread_raw(fp, params->ntt->phi);
    CHECK(fscanf(fp, "\n"), 0);
    gghlite_enc_fread_raw(fp, params->ntt->phi_inv);
}

static void fwrite_gghlite_params(FILE *fp, const gghlite_params_t params) {
    int mpfr_base = 10;
    fprintf(fp, "%zd %zd %zd %ld %ld %lu %d\n",
            params->lambda,
            params->gamma,
            params->kappa,
            params->n,
            params->ell,
            params->rerand_mask,
            params->flags
        );
    fmpz_out_raw(fp, params->q);
    fprintf(fp, "\n");
    mpfr_out_str(fp, mpfr_base, 0, params->sigma, MPFR_RNDN);
    fprintf(fp, "\n");
    mpfr_out_str(fp, mpfr_base, 0, params->sigma_p, MPFR_RNDN);
    fprintf(fp, "\n");
    mpfr_out_str(fp, mpfr_base, 0, params->sigma_s, MPFR_RNDN);
    fprintf(fp, "\n");
    mpfr_out_str(fp, mpfr_base, 0, params->ell_b, MPFR_RNDN);
    fprintf(fp, "\n");
    mpfr_out_str(fp, mpfr_base, 0, params->ell_g, MPFR_RNDN);
    fprintf(fp, "\n");
    mpfr_out_str(fp, mpfr_base, 0, params->xi, MPFR_RNDN);
    fprintf(fp, "\n");
    gghlite_enc_fprint_raw(fp, params->pzt);
    fprintf(fp, "\n");
    fprintf(fp, "%zd\n", params->ntt->n);
    fmpz_mod_poly_fprint_raw(fp, params->ntt->w);
    fprintf(fp, "\n");
    fmpz_mod_poly_fprint_raw(fp, params->ntt->w_inv);
    fprintf(fp, "\n");
    fmpz_mod_poly_fprint_raw(fp, params->ntt->phi);
    fprintf(fp, "\n");
    fmpz_mod_poly_fprint_raw(fp, params->ntt->phi_inv);
}

void fread_gghlite_sk(FILE *fp, gghlite_sk_t gghlite_self) {
    uint64_t t = ggh_walltime(0);
    timer_printf("Starting reading gghlite params...\n");
    fread_gghlite_params(fp, gghlite_self->params);
    CHECK(fscanf(fp, "\n"), 0);
    timer_printf("Finished reading gghlite params %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));

    t = ggh_walltime(0);
    timer_printf("Starting reading g, g_inv, h...\n");
    fmpz_poly_init(gghlite_self->g);
    fmpz_poly_fread_raw(fp, gghlite_self->g);
    CHECK(fscanf(fp, "\n"), 0);
    fmpq_poly_init(gghlite_self->g_inv);
    fmpq_poly_fread(fp, gghlite_self->g_inv);
    CHECK(fscanf(fp, "\n"), 0);
    fmpz_poly_init(gghlite_self->h);
    fmpz_poly_fread_raw(fp, gghlite_self->h);
    CHECK(fscanf(fp, "\n"), 0);
    timer_printf("Finished reading g, g_inv, h %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));

    t = ggh_walltime(0);
    timer_printf("Starting reading z, z_inv, a, b...\n");
    gghlite_self->z = malloc(gghlite_self->params->gamma * sizeof(gghlite_enc_t));
    gghlite_self->z_inv = malloc(gghlite_self->params->gamma * sizeof(gghlite_enc_t));
    for(unsigned int i = 0; i < gghlite_self->params->gamma; i++) {
        fmpz_t p1, p2;
        fmpz_init(p1);
        fmpz_init(p2);
        fmpz_inp_raw(p1, fp);
        CHECK(fscanf(fp, "\n"), 0);
        fmpz_mod_poly_fread_raw(fp, gghlite_self->z[i]);
        CHECK(fscanf(fp, "\n"), 0);
        fmpz_inp_raw(p2, fp);
        CHECK(fscanf(fp, "\n"), 0);
        fmpz_mod_poly_fread_raw(fp, gghlite_self->z_inv[i]);
        CHECK(fscanf(fp, "\n"), 0);
        fmpz_clear(p1);
        fmpz_clear(p2);
        timer_printf("\r    Progress: [%lu / %lu] %8.2fs",
                     i, gghlite_self->params->gamma, ggh_seconds(ggh_walltime(t)));
    }
    timer_printf("\n");
    timer_printf("Finished reading z, z_inv, a, b %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));

    t = ggh_walltime(0);
    timer_printf("Starting setting D_g...\n");
    gghlite_sk_set_D_g(gghlite_self);
    timer_printf("Finished setting D_g %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));
    aes_randstate_fread(gghlite_self->rng, fp);
}

void fwrite_gghlite_sk(FILE *fp, const gghlite_sk_t gghlite_self) {
    uint64_t t = ggh_walltime(0);
    timer_printf("Starting writing gghlite params...\n");
    fwrite_gghlite_params(fp, gghlite_self->params);
    fprintf(fp, "\n");
    timer_printf("Finished writing gghlite params %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));

    t = ggh_walltime(0);
    timer_printf("Starting writing g, g_inv, h...\n");
    fmpz_poly_fprint_raw(fp, gghlite_self->g);
    fprintf(fp, "\n");
    fmpq_poly_fprint(fp, gghlite_self->g_inv);
    fprintf(fp, "\n");
    fmpz_poly_fprint_raw(fp, gghlite_self->h);
    fprintf(fp, "\n");
    timer_printf("Finished writing g, g_inv, h %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));

    t = ggh_walltime(0);
    timer_printf("Starting writing z, z_inv, a, b...\n");
    for(unsigned int i = 0; i < gghlite_self->params->gamma; i++) {
        fmpz_out_raw(fp, fmpz_mod_poly_modulus(gghlite_self->z[i]));
        fprintf(fp, "\n");
        fmpz_mod_poly_fprint_raw(fp, gghlite_self->z[i]);
        fprintf(fp, "\n");
        fmpz_out_raw(fp, fmpz_mod_poly_modulus(gghlite_self->z_inv[i]));
        fprintf(fp, "\n");
        fmpz_mod_poly_fprint_raw(fp, gghlite_self->z_inv[i]);
        fprintf(fp, "\n");
        timer_printf("\r    Progress: [%lu / %lu] %8.2fs",
                     i, gghlite_self->params->gamma, ggh_seconds(ggh_walltime(t)));
    }
    timer_printf("\n");
    timer_printf("Finished writing z, z_inv, a, b %8.2fs\n",
                 ggh_seconds(ggh_walltime(t)));
    aes_randstate_fwrite(gghlite_self->rng, fp);
}

static int fmpz_poly_fread_raw(FILE * file, fmpz_poly_t poly)
{
    int r;
    slong i, len;
    mpz_t t;

    mpz_init(t);
    r = mpz_inp_str(t, file, 10);
    if (r == 0)
    {
        mpz_clear(t);
        return 0;
    }
    if (!mpz_fits_slong_p(t))
    {
        flint_printf("Exception (fmpz_poly_fread). Length does not fit into a slong.\n");
        abort();
    }
    len = flint_mpz_get_si(t);
    mpz_clear(t);

    fmpz_poly_fit_length(poly, len);

    for (i = 0; i < len; i++)
    {
        r = fmpz_inp_raw(poly->coeffs + i, file);
        if (r <= 0)
            return r;
    }

    _fmpz_poly_set_length(poly, len);
    _fmpz_poly_normalise(poly);

    return 1;
}


static int _fmpz_poly_fprint_raw(FILE * file, const fmpz * vec, slong len)
{
    int r;
    slong i;

    r = flint_fprintf(file, "%li", len);
    if ((len > 0) && (r > 0))
    {
        for (i = 0; (i < len) && (r > 0); i++)
        {
            r = fmpz_out_raw(file, vec + i);
        }
    }

    return r;
}

static int fmpz_poly_fprint_raw(FILE * file, const fmpz_poly_t poly)
{
    return _fmpz_poly_fprint_raw(file, poly->coeffs, poly->length);
}



static int _fmpz_mod_poly_fprint_raw(FILE * file, const fmpz *poly, slong len,
                                     const fmpz_t p)
{
    int r;
    slong i;

    r = flint_fprintf(file, "%wd ", len);
    if (r <= 0)
        return r;

    r = fmpz_out_raw(file, p);
    if (r <= 0)
        return r;

    if (len == 0)
        return r;

    r = flint_fprintf(file, " ");
    if (r <= 0)
        return r;

    for (i = 0; (r > 0) && (i < len); i++)
    {
        r = flint_fprintf(file, " ");
        if (r <= 0)
            return r;
        r = fmpz_out_raw(file, poly + i);
        if (r <= 0)
            return r;
    }

    return r;
}

static int fmpz_mod_poly_fprint_raw(FILE * file, const fmpz_mod_poly_t poly)
{
    return _fmpz_mod_poly_fprint_raw(file, poly->coeffs, poly->length,
                                     &(poly->p));
}

/* copied from fmpz_mod_poly_fread, mostly (but fixed) */
static int fmpz_mod_poly_fread_raw(FILE * f, fmpz_mod_poly_t poly)
{
    slong i, length;
    fmpz_t coeff;
    ulong res;

    fmpz_init(coeff);
    if (flint_fscanf(f, "%wd ", &length) != 1) {
        fmpz_clear(coeff);
        return 0;
    }

    fmpz_inp_raw(coeff,f);
    fmpz_mod_poly_init(poly, coeff);
    fmpz_mod_poly_fit_length(poly, length);

    poly->length = length;
    flint_fscanf(f, " ");

    for (i = 0; i < length; i++)
    {
        flint_fscanf(f, " ");
        res = fmpz_inp_raw(coeff, f);

        fmpz_mod_poly_set_coeff_fmpz(poly,i,coeff);

        if (!res)
        {
            poly->length = i;
            fmpz_clear(coeff);
            return 0;
        }
    }

    fmpz_clear(coeff);
    _fmpz_mod_poly_normalise(poly);

    return 1;
}

static const mmap_enc_vtable gghlite_enc_vtable =
{ .init = gghlite_enc_init_wrapper
  , .clear = gghlite_enc_clear_wrapper
  , .fread = gghlite_enc_fread_raw_wrapper
  , .fwrite = gghlite_enc_fprint_raw_wrapper
  , .set = gghlite_enc_set_wrapper
  , .add = gghlite_enc_add_wrapper
  , .sub = gghlite_enc_sub_wrapper
  , .mul = gghlite_enc_mul_wrapper
  , .is_zero = gghlite_enc_is_zero_wrapper
  , .encode = gghlite_enc_set_gghlite_clr_wrapper
  , .degree = NULL
  , .print = NULL
  , .size = sizeof(gghlite_enc_t)
};


const mmap_vtable gghlite_vtable =
{ .pp  = &gghlite_pp_vtable
  , .sk  = &gghlite_sk_vtable
  , .enc = &gghlite_enc_vtable
};
