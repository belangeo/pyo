/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <Python.h>
#include "vbap.h"

#define PIx2 (M_PI * 2.0)

static float ang_to_rad = (float)(2.0 * M_PI / 360.0);

/* Linked-list of all loudspeakers. */
typedef struct ls_triplet_chain
{
    int ls_nos[3];                  /* Triplet speaker numbers */
    float inv_mx[9];                /* Triplet inverse matrix */
    struct ls_triplet_chain *next;  /* Next triplet */
} ls_triplet_chain;

/* Fast-forward declarations. */
void compute_gains(int ls_set_am, LS_SET *sets, float *gains,
                   int ls_amount, CART_VEC cart_dir, int dim);

/* Returns 1 if there is loudspeaker(s) inside given ls triplet. */
static int any_ls_inside_triplet(int a, int b, int c,
                                 ls lss[MAX_LS_AMOUNT],
                                 int ls_amount)
{
    float invdet, tmp, invmx[9];
    CART_VEC *lp1, *lp2, *lp3;
    int i, j, any_ls_inside, this_inside;

    lp1 = &(lss[a].coords);
    lp2 = &(lss[b].coords);
    lp3 = &(lss[c].coords);

    /* Matrix inversion. */
    invdet = 1.0 / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
                    - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
                    + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

    invmx[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
    invmx[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
    invmx[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
    invmx[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
    invmx[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
    invmx[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
    invmx[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
    invmx[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
    invmx[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;

    any_ls_inside = 0;

    for (i = 0; i < ls_amount; i++)
    {
        if (i != a && i != b && i != c)
        {
            this_inside = 1;

            for (j = 0; j < 3; j++)
            {
                tmp = lss[i].coords.x * invmx[0 + j * 3];
                tmp += lss[i].coords.y * invmx[1 + j * 3];
                tmp += lss[i].coords.z * invmx[2 + j * 3];

                if (tmp < -0.001)
                    this_inside = 0;
            }

            if (this_inside == 1)
                any_ls_inside = 1;
        }
    }

    return any_ls_inside;
}

/* Properly free an automatically allocated ls_triplet_chain linked-list.
 */
void free_ls_triplet_chain(ls_triplet_chain *ls_triplets)
{
    ls_triplet_chain *ptr = ls_triplets, *next = ls_triplets;

    while (ptr != NULL)
    {
        next = ptr->next;
        PyMem_Free(ptr);
        ptr = next;
    }
}

/* Adds i, j, k triplet to triplet chain. */
static void add_ldsp_triplet(int i, int j, int k,
                             ls_triplet_chain **ls_triplets,
                             ls lss[MAX_LS_AMOUNT])
{
    struct ls_triplet_chain *trip_ptr, *prev;
    trip_ptr = *ls_triplets;
    prev = NULL;

    while (trip_ptr != NULL)
    {
        prev = trip_ptr;
        trip_ptr = trip_ptr->next;
    }

    trip_ptr = (ls_triplet_chain *)PyMem_Malloc(sizeof(ls_triplet_chain));

    if (prev == NULL)
        *ls_triplets = trip_ptr;
    else
        prev->next = trip_ptr;

    trip_ptr->next = NULL;
    trip_ptr->ls_nos[0] = i;
    trip_ptr->ls_nos[1] = j;
    trip_ptr->ls_nos[2] = k;
}

static float clip(float val, float min, float max)
{
    if (val < min) { return min; }
    else if (val > max) { return max; }
    else { return val; }
}

/* Returns the vector length without the sqrt. */
static float vec_long_length(CART_VEC v1)
{
    return v1.x * v1.x + v1.y * v1.y + v1.z * v1.z;
}

/* Returns the vector length. */
static float vec_length(CART_VEC v1)
{
    return sqrtf(v1.x * v1.x + v1.y * v1.y + v1.z * v1.z);
}

/* Returns the vector mean. */
static void vec_mean(CART_VEC v1, CART_VEC v2, CART_VEC *v3)
{
    v3->x = (v1.x + v2.x) * 0.5;
    v3->y = (v1.y + v2.y) * 0.5;
    v3->z = (v1.z + v2.z) * 0.5;
}

/* Returns the vector product. */
static float vec_prod(CART_VEC v1, CART_VEC v2)
{
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

static float vec_angle(CART_VEC v1, CART_VEC v2)
{
    float inner;
    //inner = (vec_prod(v1, v2) / (vec_length(v1) * vec_length(v2)));
    inner = (vec_prod(v1, v2) / sqrtf(vec_long_length(v1) * vec_long_length(v2)));
    inner = clip(inner, -1.0, 1.0);
    return fabsf(acosf(inner));
}

static void cross_prod(CART_VEC v1, CART_VEC v2, CART_VEC *res)
{
    float length;
    res->x = (v1.y * v2.z) - (v1.z * v2.y);
    res->y = (v1.z * v2.x) - (v1.x * v2.z);
    res->z = (v1.x * v2.y) - (v1.y * v2.x);
    length = vec_length(*res);
    res->x /= length;
    res->y /= length;
    res->z /= length;
}

/* Computes x,y,z coordinates from azimuth and elevation angle. */
/*
static double * angle_to_cart(float azi, float ele) {
    double *res;
    double atorad = (PIx2 / 360.0) ;
    res = (double *)PyMem_Malloc(3 * sizeof(double));
    res[0] = cos((double)(azi * atorad)) * cos((double)(ele * atorad));
    res[1] = sin((double)(azi * atorad)) * cos((double)(ele * atorad));
    res[2] = sin((double)(ele * atorad));
    return res;
}
*/

/* Converts a vector from angular to cartesian coordinates. */
static void vec_angle_to_cart(ANG_VEC *from, CART_VEC *to)
{
    float cele = cosf(from->ele * ang_to_rad);
    to->x = cosf(from->azi * ang_to_rad) * cele;
    to->y = sinf(from->azi * ang_to_rad) * cele;
    to->z = sinf(from->ele * ang_to_rad);
}

/* Check if two angle elevation coordinates are within a range of +/- 5 deg. */
static int vec_angles_are_on_same_ele(ANG_VEC v1, ANG_VEC v2)
{
    if (v1.ele > (v2.ele - 5) && v1.ele < (v2.ele + 5))
        return 1;
    else
        return 0;
}

/* Calculate volume of the parallelepiped defined by the loudspeaker
 * direction vectors and divide it with total length of the triangle sides.
 * This is used when removing too narrow triangles. */
static float vol_p_side_lgth(int i, int j, int k, ls lss[MAX_LS_AMOUNT])
{
    int same_ele;
    float volper, lgth;
    CART_VEC xprod;
    cross_prod(lss[i].coords, lss[j].coords, &xprod);
    volper = fabsf(vec_prod(xprod, lss[k].coords));
    lgth = (fabsf(vec_angle(lss[i].coords, lss[j].coords)) +
            fabsf(vec_angle(lss[i].coords, lss[k].coords)) +
            fabsf(vec_angle(lss[j].coords, lss[k].coords)));
    /* At least, two speakers should be on the same elevation plane.
       This fix wrong triplet in the Dome32Sub2UdeM template. */
    same_ele = vec_angles_are_on_same_ele(lss[i].angles, lss[j].angles) +
               vec_angles_are_on_same_ele(lss[i].angles, lss[k].angles) +
               vec_angles_are_on_same_ele(lss[j].angles, lss[k].angles);

    if (lgth > 0.00001 && same_ele > 0)
        return volper / lgth;
    else
        return 0.0;
}

/* Checks if two lines intersect on 3D sphere see theory in paper
 * Pulkki, V. Lokki, T. "Creating Auditory Displays with Multiple
 * Loudspeakers Using VBAP: A Case Study with DIVA Project" in
 * International Conference on Auditory Displays -98.
 * E-mail Ville.Pulkki@hut.fi if you want to have that paper. */
static int lines_intersect(int i, int j, int k, int l,
                           ls lss[MAX_LS_AMOUNT])
{
    CART_VEC v1, v2, v3, neg_v3;
    float dist_ij, dist_kl, dist_iv3, dist_jv3, dist_inv3, dist_jnv3;
    float dist_kv3, dist_lv3, dist_knv3, dist_lnv3;

    cross_prod(lss[i].coords, lss[j].coords, &v1);
    cross_prod(lss[k].coords, lss[l].coords, &v2);
    cross_prod(v1, v2, &v3);

    neg_v3.x = 0.0 - v3.x;
    neg_v3.y = 0.0 - v3.y;
    neg_v3.z = 0.0 - v3.z;

    dist_ij = (vec_angle(lss[i].coords, lss[j].coords));
    dist_kl = (vec_angle(lss[k].coords, lss[l].coords));
    dist_iv3 = (vec_angle(lss[i].coords, v3));
    dist_jv3 = (vec_angle(v3, lss[j].coords));
    dist_inv3 = (vec_angle(lss[i].coords, neg_v3));
    dist_jnv3 = (vec_angle(neg_v3, lss[j].coords));
    dist_kv3 = (vec_angle(lss[k].coords, v3));
    dist_lv3 = (vec_angle(v3, lss[l].coords));
    dist_knv3 = (vec_angle(lss[k].coords, neg_v3));
    dist_lnv3 = (vec_angle(neg_v3, lss[l].coords));

    /*One of loudspeakers is close to crossing point, don't do anything.*/
    if (fabsf(dist_iv3) <= 0.01 || fabsf(dist_jv3) <= 0.01 ||
            fabsf(dist_kv3) <= 0.01 || fabsf(dist_lv3) <= 0.01 ||
            fabsf(dist_inv3) <= 0.01 || fabsf(dist_jnv3) <= 0.01 ||
            fabsf(dist_knv3) <= 0.01 || fabsf(dist_lnv3) <= 0.01)
    {
        return (0);
    }

    if (((fabsf(dist_ij - (dist_iv3 + dist_jv3)) <= 0.01 ) &&
            (fabsf(dist_kl - (dist_kv3 + dist_lv3))  <= 0.01)) ||
            ((fabsf(dist_ij - (dist_inv3 + dist_jnv3)) <= 0.01)  &&
             (fabsf(dist_kl - (dist_knv3 + dist_lnv3)) <= 0.01 )))
    {
        return (1);
    }
    else
    {
        return (0);
    }
}

/* subroutine for spreading */
static void new_spread_dir(CART_VEC *spreaddir, CART_VEC vscartdir,
                           CART_VEC spread_base, float azi, float spread)
{
    float a, b, beta, gamma, power, sum;
    ANG_VEC tmp;

    sum = clip(vec_prod(vscartdir, spread_base), -1.0, 1.0);
    gamma = acosf(sum) / M_PI * 180.0;

    if(fabsf(gamma) < 1)
    {
        tmp.azi = azi + 90.0;
        tmp.ele = 0.0;
        tmp.length = 1.0;
        vec_angle_to_cart(&tmp, &spread_base);
        sum = clip(vec_prod(vscartdir, spread_base), -1.0, 1.0);
        gamma = acosf(sum) / M_PI * 180.0;
    }

    beta = 180.0 - gamma;
    b = sinf(spread * M_PI / 180.0) / sinf(beta * M_PI / 180.0);
    a = sinf((180.0 - spread - beta) * M_PI / 180.0) /
        sinf(beta * M_PI / 180.0);
    spreaddir->x = a * vscartdir.x + b * spread_base.x;
    spreaddir->y = a * vscartdir.y + b * spread_base.y;
    spreaddir->z = a * vscartdir.z + b * spread_base.z;

    power = sqrtf(vec_prod(*spreaddir, *spreaddir));
    spreaddir->x /= power;
    spreaddir->y /= power;
    spreaddir->z /= power;
}

/* subroutine for spreading */
static void new_spread_base(CART_VEC spreaddir, CART_VEC vscartdir,
                            float spread, CART_VEC *spread_base)
{
    float d, power;

    d = cosf(spread / 180.0 * M_PI);
    spread_base->x = spreaddir.x - d * vscartdir.x;
    spread_base->y = spreaddir.y - d * vscartdir.y;
    spread_base->z = spreaddir.z - d * vscartdir.z;
    power = sqrtf(vec_prod(*spread_base, *spread_base));
    spread_base->x /= power;
    spread_base->y /= power;
    spread_base->z /= power;
}

/*
 * apply the sound signal to multiple panning directions
 * that causes some spreading.
 * See theory in paper V. Pulkki "Uniform spreading of amplitude panned
 * virtual sources" in WASPAA 99
 */
static void spreadit(float azi, float spread, VBAP_DATA *data)
{
    int j;
    CART_VEC spreaddir[16];
    CART_VEC spreadbase[16];
    int i, spreaddirnum = 16;
    int cnt = data->ls_am;
    float tmp_gains[cnt];

    for (i = 0; i < cnt; i++)
    {
        tmp_gains[i] = 0.0;
    }

    float sum = 0.0;

    /* four orthogonal dirs */
    new_spread_dir(&spreaddir[0], data->cart_dir, data->spread_base,
                   azi, spread);
    new_spread_base(spreaddir[0], data->cart_dir, spread,
                    &data->spread_base);
    cross_prod(data->spread_base, data->cart_dir, &spreadbase[1]);
    cross_prod(spreadbase[1], data->cart_dir, &spreadbase[2]);
    cross_prod(spreadbase[2], data->cart_dir, &spreadbase[3]);

    /* four between them */
    vec_mean(data->spread_base, spreadbase[1], &spreadbase[4]);
    vec_mean(spreadbase[1], spreadbase[2], &spreadbase[5]);
    vec_mean(spreadbase[2], spreadbase[3], &spreadbase[6]);
    vec_mean(spreadbase[3], data->spread_base, &spreadbase[7]);

    /* four at half spread angle */
    vec_mean(data->cart_dir, data->spread_base, &spreadbase[8]);
    vec_mean(data->cart_dir, spreadbase[1], &spreadbase[9]);
    vec_mean(data->cart_dir, spreadbase[2], &spreadbase[10]);
    vec_mean(data->cart_dir, spreadbase[3], &spreadbase[11]);

    /* four at quarter spread angle */
    vec_mean(data->cart_dir, spreadbase[8], &spreadbase[12]);
    vec_mean(data->cart_dir, spreadbase[9], &spreadbase[13]);
    vec_mean(data->cart_dir, spreadbase[10], &spreadbase[14]);
    vec_mean(data->cart_dir, spreadbase[11], &spreadbase[15]);

    for (i = 1; i < spreaddirnum; i++)
    {
        new_spread_dir(&spreaddir[i], data->cart_dir, spreadbase[i],
                       azi, spread);
        compute_gains(data->ls_set_am, data->ls_sets, tmp_gains,
                      data->ls_am, spreaddir[i], data->dimension);

        for (j = 0; j < cnt; j++)
        {
            data->gains[j] += tmp_gains[j];
        }
    }

    if (spread > 70.0)
    {
        for (i = 0; i < cnt; i++)
        {
            data->gains[i] += (spread - 70.0) / 30.0 *
                              (spread - 70.0) / 30.0 * 20.0;
        }
    }

    for (i = 0; i < cnt; i++)
    {
        sum += (data->gains[i] * data->gains[i]);
    }

    sum = sqrtf(sum);

    for (i = 0; i < cnt; i++)
    {
        data->gains[i] /= sum;
    }
}

static void spreadit_azi_ele(float azi, float ele, float sp_azi,
                             float sp_ele, VBAP_DATA *data)
{
    int i, j, k, ind, num = 4, knum = 4;
    float azidev, eledev, newazi, newele, comp;
    ANG_VEC spreadang;
    CART_VEC spreadcart;
    int cnt = data->ls_am;
    float tmp_gains[cnt];

    for (i = 0; i < cnt; i++)
    {
        tmp_gains[i] = 0.0;
    }

    float sum = 0.0;

    if (sp_azi < 0.0) { sp_azi = 0.0; }
    else if (sp_azi > 1.0) { sp_azi = 1.0; }

    if (sp_ele < 0.0) { sp_ele = 0.0; }
    else if (sp_ele > 1.0) { sp_ele = 1.0; }

    // If both sp_azi and sp_ele are active, we want to put a virtual source at
    // (azi, ele +/- eledev) and (azi +/- azidev, ele) locations.
    if (sp_azi > 0.0 && sp_ele > 0.0)
    {
        knum = 8;
    }
    else
    {
        knum = 4;
    }

    for (i = 0; i < num; i++)
    {
        comp = powf(10.0f, (i + 1) * -3.0f * 0.05f);
        azidev = (i + 1) * sp_azi * 45.0;
        eledev = (i + 1) * sp_ele * 22.5;

        for (k = 0; k < knum; k++)
        {
            if (k == 0)
            {
                newazi = data->ang_dir.azi + azidev;
                newele = data->ang_dir.ele + eledev;
            }
            else if (k == 1)
            {
                newazi = data->ang_dir.azi - azidev;
                newele = data->ang_dir.ele - eledev;
            }
            else if (k == 2)
            {
                newazi = data->ang_dir.azi + azidev;
                newele = data->ang_dir.ele - eledev;
            }
            else if (k == 3)
            {
                newazi = data->ang_dir.azi - azidev;
                newele = data->ang_dir.ele + eledev;
            }
            else if (k == 4)
            {
                newazi = data->ang_dir.azi;
                newele = data->ang_dir.ele + eledev;
            }
            else if (k == 5)
            {
                newazi = data->ang_dir.azi;
                newele = data->ang_dir.ele - eledev;
            }
            else if (k == 6)
            {
                newazi = data->ang_dir.azi + azidev;
                newele = data->ang_dir.ele;
            }
            else if (k == 7)
            {
                newazi = data->ang_dir.azi - azidev;
                newele = data->ang_dir.ele;
            }

            if (newazi > 180) { newazi -= 360; }
            else if (newazi < -180) { newazi += 360; }

            if (newele > 90) { newele = 90; }
            else if (newele < 0) { newele = 0; }

            spreadang.azi = newazi;
            spreadang.ele = newele;
            spreadang.length = 1.0;
            vec_angle_to_cart(&spreadang, &spreadcart);
            compute_gains(data->ls_set_am, data->ls_sets, tmp_gains,
                          data->ls_am, spreadcart, data->dimension);

            for (j = 0; j < cnt; j++)
            {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    if (sp_azi > 0.8 && sp_ele > 0.8)
    {
        comp = (sp_azi - 0.8) / 0.2 * (sp_ele - 0.8) / 0.2 * 10.0;

        for (i = 0; i < data->ls_out; i++)
        {
            data->gains[data->out_patches[i] - 1] += comp;
        }
    }

    for (i = 0; i < data->ls_out; i++)
    {
        ind = data->out_patches[i] - 1;
        sum += (data->gains[ind] * data->gains[ind]);
    }

    sum = sqrtf(sum);

    for (i = 0; i < data->ls_out; i++)
    {
        ind = data->out_patches[i] - 1;
        data->gains[ind] /= sum;
    }
}

static void spreadit_azi_ele_flip_y_z(float azi, float ele, float sp_azi,
                                      float sp_ele, VBAP_DATA *data)
{
    int i, j, k, ind, num = 4, knum = 4;
    float azidev, eledev, newazi, newele, comp, tmp;
    ANG_VEC spreadang;
    CART_VEC spreadcart;
    int cnt = data->ls_am;
    float tmp_gains[cnt];

    for (i = 0; i < cnt; i++)
    {
        tmp_gains[i] = 0.0;
    }

    float sum = 0.0;

    if (sp_azi < 0.0) { sp_azi = 0.0; }
    else if (sp_azi > 1.0) { sp_azi = 1.0; }

    if (sp_ele < 0.0) { sp_ele = 0.0; }
    else if (sp_ele > 1.0) { sp_ele = 1.0; }

    // If both sp_azi and sp_ele are active, we want to put a virtual source at
    // (azi, ele +/- eledev) and (azi +/- azidev, ele) locations.
    if (sp_azi > 0.0 && sp_ele > 0.0)
    {
        knum = 8;
    }
    else
    {
        knum = 4;
    }

    for (i = 0; i < num; i++)
    {
        comp = powf(10.0f, (i + 1) * -3.0f * 0.05f);
        azidev = (i + 1) * sp_azi * 45.0;
        eledev = (i + 1) * sp_ele * 22.5;

        for (k = 0; k < knum; k++)
        {
            if (k == 0)
            {
                newazi = data->ang_dir.azi + azidev;
                newele = data->ang_dir.ele + eledev;
            }
            else if (k == 1)
            {
                newazi = data->ang_dir.azi - azidev;
                newele = data->ang_dir.ele - eledev;
            }
            else if (k == 2)
            {
                newazi = data->ang_dir.azi + azidev;
                newele = data->ang_dir.ele - eledev;
            }
            else if (k == 3)
            {
                newazi = data->ang_dir.azi - azidev;
                newele = data->ang_dir.ele + eledev;
            }
            else if (k == 4)
            {
                newazi = data->ang_dir.azi;
                newele = data->ang_dir.ele + eledev;
            }
            else if (k == 5)
            {
                newazi = data->ang_dir.azi;
                newele = data->ang_dir.ele - eledev;
            }
            else if (k == 6)
            {
                newazi = data->ang_dir.azi + azidev;
                newele = data->ang_dir.ele;
            }
            else if (k == 7)
            {
                newazi = data->ang_dir.azi - azidev;
                newele = data->ang_dir.ele;
            }

            if (newazi > 180) { newazi -= 360; }
            else if (newazi < -180) { newazi += 360; }

            if (newele > 90) { newele = 90; }
            else if (newele < 0) { newele = 0; }

            spreadang.azi = newazi;
            spreadang.ele = newele;
            spreadang.length = 1.0;
            vec_angle_to_cart(&spreadang, &spreadcart);
            tmp = spreadcart.z;
            spreadcart.z = spreadcart.y;
            spreadcart.y = tmp;
            compute_gains(data->ls_set_am, data->ls_sets, tmp_gains,
                          data->ls_am, spreadcart, data->dimension);

            for (j = 0; j < cnt; j++)
            {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    if (sp_azi > 0.8 && sp_ele > 0.8)
    {
        comp = (sp_azi - 0.8) / 0.2 * (sp_ele - 0.8) / 0.2 * 10.0;

        for (i = 0; i < data->ls_out; i++)
        {
            data->gains[data->out_patches[i] - 1] += comp;
        }
    }

    for (i = 0; i < data->ls_out; i++)
    {
        ind = data->out_patches[i] - 1;
        sum += (data->gains[ind] * data->gains[ind]);
    }

    sum = sqrtf(sum);

    for (i = 0; i < data->ls_out; i++)
    {
        ind = data->out_patches[i] - 1;
        data->gains[ind] /= sum;
    }
}

static void spreadit_azi(float azi, float sp_azi, VBAP_DATA *data)
{
    int i, j, k, num = 4;
    float azidev, newazi, comp;
    ANG_VEC spreadang;
    CART_VEC spreadcart;
    int cnt = data->ls_am;
    float tmp_gains[cnt];

    for (i = 0; i < cnt; i++)
    {
        tmp_gains[i] = 0.0;
    }

    float sum = 0.0;

    if (sp_azi < 0.0) { sp_azi = 0.0; }
    else if (sp_azi > 1.0) { sp_azi = 1.0; }

    for (i = 0; i < num; i++)
    {
        comp = powf(10.0f, (i + 1) * -3.0f * 0.05f);
        azidev = (i + 1) * sp_azi * 45.0;

        for (k = 0; k < 2; k++)
        {
            if (k == 0)
            {
                newazi = data->ang_dir.azi + azidev;
            }
            else if (k == 1)
            {
                newazi = data->ang_dir.azi - azidev;
            }

            if (newazi > 180) { newazi -= 360; }
            else if (newazi < -180) { newazi += 360; }

            spreadang.azi = newazi;
            spreadang.ele = 0.0;
            spreadang.length = 1.0;
            vec_angle_to_cart(&spreadang, &spreadcart);
            compute_gains(data->ls_set_am, data->ls_sets, tmp_gains,
                          data->ls_am, spreadcart, data->dimension);

            for (j = 0; j < cnt; j++)
            {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    for (i = 0; i < cnt; i++)
    {
        sum += (data->gains[i] * data->gains[i]);
    }

    sum = sqrtf(sum);

    for (i = 0; i < cnt; i++)
    {
        data->gains[i] /= sum;
    }
}

static void spreadit_azi_flip_y_z(float azi, float sp_azi, VBAP_DATA *data)
{
    int i, j, k, num = 4;
    float azidev, newazi, comp, tmp;
    ANG_VEC spreadang;
    CART_VEC spreadcart;
    int cnt = data->ls_am;
    float tmp_gains[cnt];

    for (i = 0; i < cnt; i++)
    {
        tmp_gains[i] = 0.0;
    }

    float sum = 0.0;

    if (sp_azi < 0.0) { sp_azi = 0.0; }
    else if (sp_azi > 1.0) { sp_azi = 1.0; }

    for (i = 0; i < num; i++)
    {
        comp = powf(10.0f, (i + 1) * -3.0f * 0.05f);
        azidev = (i + 1) * sp_azi * 45.0;

        for (k = 0; k < 2; k++)
        {
            if (k == 0)
            {
                newazi = data->ang_dir.azi + azidev;
            }
            else if (k == 1)
            {
                newazi = data->ang_dir.azi - azidev;
            }

            if (newazi > 180) { newazi -= 360; }
            else if (newazi < -180) { newazi += 360; }

            spreadang.azi = newazi;
            spreadang.ele = 0.0;
            spreadang.length = 1.0;
            vec_angle_to_cart(&spreadang, &spreadcart);
            tmp = spreadcart.z;
            spreadcart.z = spreadcart.y;
            spreadcart.y = tmp;
            compute_gains(data->ls_set_am, data->ls_sets, tmp_gains,
                          data->ls_am, spreadcart, data->dimension);

            for (j = 0; j < cnt; j++)
            {
                data->gains[j] += (tmp_gains[j] * comp);
            }
        }
    }

    for (i = 0; i < cnt; i++)
    {
        sum += (data->gains[i] * data->gains[i]);
    }

    sum = sqrtf(sum);

    for (i = 0; i < cnt; i++)
    {
        data->gains[i] /= sum;
    }
}

void free_speakers_setup(SPEAKERS_SETUP *setup)
{
    PyMem_Free(setup->azimuth);
    PyMem_Free(setup->elevation);
    PyMem_Free(setup);
}

SPEAKERS_SETUP *
load_speakers_setup(int count, float *azi, float *ele)
{
    int i;
    SPEAKERS_SETUP *setup;
    setup = (SPEAKERS_SETUP *)PyMem_Malloc(sizeof(SPEAKERS_SETUP));

    if (count < 3)
    {
        fprintf(stderr, "Too few loudspeakers %d\n", count);
        PyMem_Free(setup);
        exit(-1);
    }

    setup->azimuth = (float *)calloc(count, sizeof(float));
    setup->elevation = (float *)calloc(count, sizeof(float));

    for (i = 0; i < count; i++)
    {
        setup->azimuth[i] = azi[i];
        setup->elevation[i] = ele[i];
    }

    setup->dimension = 3;
    setup->count = count;
    return setup;
}

SPEAKERS_SETUP *
load_speakers_setup_from_file(const char *filename)
{
    int i = 0, count;
    float azi, ele;
    char *toke;
    char c[10000];
    FILE *fp;
    SPEAKERS_SETUP *setup = NULL;
    setup = (SPEAKERS_SETUP *)PyMem_Malloc(sizeof(SPEAKERS_SETUP));

    if ((fp = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "Could not open loudspeaker setup file.\n");
        PyMem_Free(setup);
        exit(-1);
    }

    if ( fgets(c, 10000, fp) != NULL )
    {
        toke = (char *)strtok(c, " ");
        sscanf(toke, "%d", &count);

        if (count < 3)
        {
            fprintf(stderr, "Too few loudspeakers %d\n", count);
            PyMem_Free(setup);
            exit(-1);
        }

        setup->azimuth = (float *)calloc(count, sizeof(float));
        setup->elevation = (float *)calloc(count, sizeof(float));

        while (1)
        {
            if (fgets(c, 10000, fp) == NULL)
                break;

            toke = (char *)strtok(c, " ");

            if (sscanf(toke, "%f", &azi) > 0)
            {
                toke = (char *)strtok(NULL, " ");
                sscanf(toke, "%f", &ele);
            }
            else
            {
                break;
            }

            setup->azimuth[i] = azi;
            setup->elevation[i] = ele;
            i++;

            if (i == count)
                break;
        }

        setup->dimension = 3;
        setup->count = count;
    }

    return setup;
}

/* Build a speakers list from a SPEAKERS_SETUP structure.
 */
void build_speakers_list(SPEAKERS_SETUP *setup, ls lss[MAX_LS_AMOUNT])
{
    int i;
    ANG_VEC a_vector;
    CART_VEC c_vector;

    for (i = 0; i < setup->count; i++)
    {
        a_vector.azi = setup->azimuth[i];
        a_vector.ele = setup->elevation[i];
        vec_angle_to_cart(&a_vector, &c_vector);
        lss[i].coords.x = c_vector.x;
        lss[i].coords.y = c_vector.y;
        lss[i].coords.z = c_vector.z;
        lss[i].angles.azi = a_vector.azi;
        lss[i].angles.ele = a_vector.ele;
        lss[i].angles.length = 1.0;
    }
}

/*
 * No external use.
 */
void sort_2D_lss(ls lss[MAX_LS_AMOUNT],
                 int sorted_lss[MAX_LS_AMOUNT],
                 int ls_amount)
{
    int i, j, index = 0;
    float tmp, tmp_azi;

    /* Transforming angles between -180 and 180. */
    for (i = 0; i < ls_amount; i++)
    {
        vec_angle_to_cart(&lss[i].angles, &lss[i].coords);
        lss[i].angles.azi = acosf(lss[i].coords.x);

        if (fabsf(lss[i].coords.y) <= 0.001)
            tmp = 1.0;
        else
            tmp = lss[i].coords.y / fabsf(lss[i].coords.y);

        lss[i].angles.azi *= tmp;
    }

    for (i = 0; i < ls_amount; i++)
    {
        tmp = 2000;

        for (j = 0 ; j < ls_amount; j++)
        {
            if (lss[j].angles.azi <= tmp)
            {
                tmp = lss[j].angles.azi;
                index = j;
            }
        }

        sorted_lss[i] = index;
        tmp_azi = lss[index].angles.azi;
        lss[index].angles.azi = (tmp_azi + 4000.0);
    }

    for (i = 0; i < ls_amount; i++)
    {
        tmp_azi = lss[i].angles.azi;
        lss[i].angles.azi = (tmp_azi - 4000.0);
    }
}

/*
 * No external use.
 */
int calc_2D_inv_tmatrix(float azi1, float azi2, float inv_mat[4])
{
    float x1, x2, x3, x4; /* x1 x3 */
    float det;
    x1 = cosf(azi1);
    x2 = sinf(azi1);
    x3 = cosf(azi2);
    x4 = sinf(azi2);
    det = (x1 * x4) - (x3 * x2);

    if (fabsf(det) <= 0.001)
    {
        inv_mat[0] = 0.0;
        inv_mat[1] = 0.0;
        inv_mat[2] = 0.0;
        inv_mat[3] = 0.0;
        return 0;
    }
    else
    {
        inv_mat[0] = x4 / det;
        inv_mat[1] = -x3 / det;
        inv_mat[2] = -x2 / det;
        inv_mat[3] = x1 / det;
        return 1;
    }
}

/* Selects the loudspeaker pairs, calculates the inversion
 * matrices and stores the data to a global array.
 */
void choose_ls_tuplets(ls lss[MAX_LS_AMOUNT],
                       ls_triplet_chain **ls_triplets,
                       int ls_amount)
{
    int i, j, amount = 0;
    int sorted_lss[MAX_LS_AMOUNT];
    int exist[MAX_LS_AMOUNT];
    float inv_mat[MAX_LS_AMOUNT][4];
    struct ls_triplet_chain *prev, *tr_ptr = *ls_triplets;
    prev = NULL;

    for (i = 0; i < MAX_LS_AMOUNT; i++)
    {
        exist[i] = 0;
    }

    /* Sort loudspeakers according their azimuth angle. */
    sort_2D_lss(lss, sorted_lss, ls_amount);

    /* Adjacent loudspeakers are the loudspeaker pairs to be used. */
    for (i = 0; i < (ls_amount - 1); i++)
    {
        if ((lss[sorted_lss[i + 1]].angles.azi -
                lss[sorted_lss[i]].angles.azi) <= (M_PI - 0.175f))
        {
            if (calc_2D_inv_tmatrix(lss[sorted_lss[i]].angles.azi,
                                    lss[sorted_lss[i + 1]].angles.azi,
                                    inv_mat[i]) != 0)
            {
                exist[i] = 1;
                amount++;
            }
        }
    }

    if (((PIx2 - lss[sorted_lss[ls_amount - 1]].angles.azi) +
            lss[sorted_lss[0]].angles.azi) <= (M_PI - 0.175f))
    {
        if (calc_2D_inv_tmatrix(lss[sorted_lss[ls_amount - 1]].angles.azi,
                                lss[sorted_lss[0]].angles.azi,
                                inv_mat[ls_amount - 1]) != 0)
        {
            exist[ls_amount - 1] = 1;
            amount++;
        }
    }

    for (i = 0; i < ls_amount - 1; i++)
    {
        if(exist[i] == 1)
        {
            while (tr_ptr != NULL)
            {
                prev = tr_ptr;
                tr_ptr = tr_ptr->next;
            }

            tr_ptr = (struct ls_triplet_chain *)PyMem_Malloc(sizeof(struct ls_triplet_chain));

            if (prev == NULL)
                *ls_triplets = tr_ptr;
            else
                prev->next = tr_ptr;

            tr_ptr->next = NULL;
            tr_ptr->ls_nos[0] = sorted_lss[i] + 1;
            tr_ptr->ls_nos[1] = sorted_lss[i + 1] + 1;

            for (j = 0; j < 4; j++)
            {
                tr_ptr->inv_mx[j] = inv_mat[i][j];
            }
        }
    }

    if(exist[ls_amount - 1] == 1)
    {
        while (tr_ptr != NULL)
        {
            prev = tr_ptr;
            tr_ptr = tr_ptr->next;
        }

        tr_ptr = (struct ls_triplet_chain *)PyMem_Malloc(sizeof(struct ls_triplet_chain));

        if (prev == NULL)
            *ls_triplets = tr_ptr;
        else
            prev->next = tr_ptr;

        tr_ptr->next = NULL;
        tr_ptr->ls_nos[0] = sorted_lss[ls_amount - 1] + 1;
        tr_ptr->ls_nos[1] = sorted_lss[0] + 1;

        for (j = 0; j < 4; j++)
        {
            tr_ptr->inv_mx[j] = inv_mat[ls_amount - 1][j];
        }
    }
}

/* Selects the loudspeaker triplets, and calculates the inversion
 * matrices for each selected triplet. A line (connection) is drawn
 * between each loudspeaker. The lines denote the sides of the
 * triangles. The triangles should not be intersecting. All crossing
 * connections are searched and the longer connection is erased.
 * This yields non-intesecting triangles, which can be used in panning.
 */
void choose_ls_triplets(ls lss[MAX_LS_AMOUNT],
                        ls_triplet_chain **ls_triplets,
                        int ls_amount)
{
    int i, j, k, l, table_size;
    int connections[MAX_LS_AMOUNT][MAX_LS_AMOUNT];
    float distance_table[((MAX_LS_AMOUNT * (MAX_LS_AMOUNT - 1)) / 2)];
    int distance_table_i[((MAX_LS_AMOUNT * (MAX_LS_AMOUNT - 1)) / 2)];
    int distance_table_j[((MAX_LS_AMOUNT * (MAX_LS_AMOUNT - 1)) / 2)];
    float distance;
    ls_triplet_chain *trip_ptr, *prev, *tmp_ptr;

    if (ls_amount == 0)
    {
        fprintf(stderr, "Number of loudspeakers is zero.\nExiting!\n");
        exit(-1);
    }

    for (i = 0; i < ls_amount; i++)
    {
        for (j = i + 1; j < ls_amount; j++)
        {
            for (k = j + 1; k < ls_amount; k++)
            {
                if(vol_p_side_lgth(i, j, k, lss) > MIN_VOL_P_SIDE_LGTH)
                {
                    connections[i][j] = 1;
                    connections[j][i] = 1;
                    connections[i][k] = 1;
                    connections[k][i] = 1;
                    connections[j][k] = 1;
                    connections[k][j] = 1;
                    add_ldsp_triplet(i, j, k, ls_triplets, lss);
                }
            }
        }
    }

    /* Calculate distancies between all lss and sorting them. */
    table_size = (((ls_amount - 1) * (ls_amount)) / 2);

    for (i = 0; i < table_size; i++)
    {
        distance_table[i] = 100000.0;
    }

    for (i = 0; i < ls_amount; i++)
    {
        for (j = (i + 1); j < ls_amount; j++)
        {
            if(connections[i][j] == 1)
            {
                distance = fabs(vec_angle(lss[i].coords, lss[j].coords));
                k = 0;

                while(distance_table[k] < distance)
                {
                    k++;
                }

                for (l = (table_size - 1); l > k ; l--)
                {
                    distance_table[l] = distance_table[l - 1];
                    distance_table_i[l] = distance_table_i[l - 1];
                    distance_table_j[l] = distance_table_j[l - 1];
                }

                distance_table[k] = distance;
                distance_table_i[k] = i;
                distance_table_j[k] = j;
            }
            else
            {
                table_size--;
            }
        }
    }

    /* Disconnecting connections which are crossing shorter ones,
     * starting from shortest one and removing all that cross it,
     * and proceeding to next shortest. */
    for (i = 0; i < (table_size); i++)
    {
        int fst_ls = distance_table_i[i];
        int sec_ls = distance_table_j[i];

        if (connections[fst_ls][sec_ls] == 1)
        {
            for (j = 0; j < ls_amount; j++)
            {
                for (k = j + 1; k < ls_amount; k++)
                {
                    if ((j != fst_ls) && (k != sec_ls) &&
                            (k != fst_ls) && (j != sec_ls))
                    {
                        if (lines_intersect(fst_ls, sec_ls, j, k, lss))
                        {
                            connections[j][k] = 0;
                            connections[k][j] = 0;
                        }
                    }
                }
            }
        }
    }

    /* Remove triangles which had crossing sides with
     * smaller triangles or include loudspeakers. */
    trip_ptr = *ls_triplets;
    prev = NULL;

    while (trip_ptr != NULL)
    {
        i = trip_ptr->ls_nos[0];
        j = trip_ptr->ls_nos[1];
        k = trip_ptr->ls_nos[2];

        if (connections[i][j] == 0 ||
                connections[i][k] == 0 ||
                connections[j][k] == 0 ||
                any_ls_inside_triplet(i, j, k, lss, ls_amount) == 1 )
        {
            if (prev != NULL)
            {
                prev->next = trip_ptr->next;
                tmp_ptr = trip_ptr;
                trip_ptr = trip_ptr->next;
                PyMem_Free(tmp_ptr);
            }
            else
            {
                *ls_triplets = trip_ptr->next;
                tmp_ptr = trip_ptr;
                trip_ptr = trip_ptr->next;
                PyMem_Free(tmp_ptr);
            }
        }
        else
        {
            prev = trip_ptr;
            trip_ptr = trip_ptr->next;
        }
    }
}

/* Calculates the inverse matrices for 3D.
 *
 * After this call, ls_triplets contains the speakers numbers
 * and the inverse matrix needed to compute channel gains.
 */
int calculate_3x3_matrixes(ls_triplet_chain *ls_triplets,
                           ls lss[MAX_LS_AMOUNT], int ls_amount)
{
    float invdet;
    CART_VEC *lp1, *lp2, *lp3;
    float *invmx;
    ls_triplet_chain *tr_ptr = ls_triplets;

    if (tr_ptr == NULL)
    {
        fprintf(stderr, "Not valid 3-D configuration.\n");
        return 0;
    }

    /* Calculations and data storage. */
    while(tr_ptr != NULL)
    {
        lp1 = &(lss[tr_ptr->ls_nos[0]].coords);
        lp2 = &(lss[tr_ptr->ls_nos[1]].coords);
        lp3 = &(lss[tr_ptr->ls_nos[2]].coords);

        /* Matrix inversion. */
        invmx = tr_ptr->inv_mx;
        invdet = 1.0 / (lp1->x * ((lp2->y * lp3->z) - (lp2->z * lp3->y))
                        - lp1->y * ((lp2->x * lp3->z) - (lp2->z * lp3->x))
                        + lp1->z * ((lp2->x * lp3->y) - (lp2->y * lp3->x)));

        invmx[0] = ((lp2->y * lp3->z) - (lp2->z * lp3->y)) * invdet;
        invmx[3] = ((lp1->y * lp3->z) - (lp1->z * lp3->y)) * -invdet;
        invmx[6] = ((lp1->y * lp2->z) - (lp1->z * lp2->y)) * invdet;
        invmx[1] = ((lp2->x * lp3->z) - (lp2->z * lp3->x)) * -invdet;
        invmx[4] = ((lp1->x * lp3->z) - (lp1->z * lp3->x)) * invdet;
        invmx[7] = ((lp1->x * lp2->z) - (lp1->z * lp2->x)) * -invdet;
        invmx[2] = ((lp2->x * lp3->y) - (lp2->y * lp3->x)) * invdet;
        invmx[5] = ((lp1->x * lp3->y) - (lp1->y * lp3->x)) * -invdet;
        invmx[8] = ((lp1->x * lp2->y) - (lp1->y * lp2->x)) * invdet;
        tr_ptr = tr_ptr->next;
    }

    return 1;
}

/* To be implemented without file reading...
 * Load explicit speakers triplets. Not tested yet...
 */
void load_ls_triplets(ls lss[MAX_LS_AMOUNT],
                      ls_triplet_chain **ls_triplets,
                      int ls_amount, const char *filename)
{
    ls_triplet_chain *trip_ptr, *prev;
    int i, j, k;
    FILE *fp;
    char c[10000];
    char *toke;

    trip_ptr = *ls_triplets;
    prev = NULL;

    while (trip_ptr != NULL)
    {
        prev = trip_ptr;
        trip_ptr = trip_ptr->next;
    }

    if ((fp = fopen(filename, "r")) == NULL)
    {
        fprintf(stderr, "Could not open loudspeaker setup file.\n");
        exit(-1);
    }

    while(1)
    {
        if(fgets(c, 10000, fp) == NULL)
            break;

        toke = (char *)strtok(c, " ");

        if(sscanf(toke, "%d", &i) > 0)
        {
            toke = (char *)strtok(NULL, " ");
            sscanf(toke, "%d", &j);
            toke = (char *)strtok(NULL, " ");
            sscanf(toke, "%d", &k);
        }
        else
        {
            break;
        }

        trip_ptr = (ls_triplet_chain *)PyMem_Malloc(sizeof(ls_triplet_chain));

        if (prev == NULL)
            *ls_triplets = trip_ptr;
        else
            prev->next = trip_ptr;

        trip_ptr->next = NULL;
        trip_ptr->ls_nos[0] = i - 1;
        trip_ptr->ls_nos[1] = j - 1;
        trip_ptr->ls_nos[2] = k - 1;
        prev = trip_ptr;
        trip_ptr = NULL;
    }
}

VBAP_DATA * init_vbap_data(SPEAKERS_SETUP *setup, int **triplets)
{
    int i, j, ret;
    ls lss[MAX_LS_AMOUNT];
    ls_triplet_chain *ls_triplets = NULL;
    ls_triplet_chain *ls_ptr;
    VBAP_DATA *data = (VBAP_DATA *)PyMem_Malloc(sizeof(VBAP_DATA));

    build_speakers_list(setup, lss);

    if (triplets == NULL)
        choose_ls_triplets(lss, &ls_triplets, setup->count);
    else
        load_ls_triplets(lss, &ls_triplets, setup->count, "filename");

    ret = calculate_3x3_matrixes(ls_triplets, lss, setup->count);

    if (ret == 0)
    {
        PyMem_Free(data);
        return NULL;
    }

    data->dimension = setup->dimension;
    data->ls_am = setup->count;

    for (i = 0; i < MAX_LS_AMOUNT; i++)
    {
        data->gains[i] = data->y[i] = 0.0;
    }

    i = 0;
    ls_ptr = ls_triplets;

    while (ls_ptr != NULL)
    {
        ls_ptr = ls_ptr->next;
        i++;
    }

    data->ls_set_am = i;
    data->ls_sets = (LS_SET *)PyMem_Malloc(sizeof(LS_SET) * i);

    i = 0;
    ls_ptr = ls_triplets;

    while (ls_ptr != NULL)
    {
        for (j = 0; j < data->dimension; j++)
        {
            data->ls_sets[i].ls_nos[j] = ls_ptr->ls_nos[j] + 1;
        }

        for (j = 0; j < (data->dimension * data->dimension); j++)
        {
            data->ls_sets[i].inv_mx[j] = ls_ptr->inv_mx[j];
        }

        ls_ptr = ls_ptr->next;
        i++;
    }

    free_ls_triplet_chain(ls_triplets);

    return data;
}

VBAP_DATA * init_vbap_from_speakers(ls lss[MAX_LS_AMOUNT], int count,
                                    int dim, int outputPatches[MAX_LS_AMOUNT],
                                    int maxOutputPatch, int **triplets)
{
    int i, j, ret, offset = 0;
    ls_triplet_chain *ls_triplets = NULL;
    ls_triplet_chain *ls_ptr;
    VBAP_DATA *data = (VBAP_DATA *)PyMem_Malloc(sizeof(VBAP_DATA));

    if (dim == 3)
    {
        if (triplets == NULL)
            choose_ls_triplets(lss, &ls_triplets, count);
        else
            load_ls_triplets(lss, &ls_triplets, count, "filename");

        ret = calculate_3x3_matrixes(ls_triplets, lss, count);

        if (ret == 0)
        {
            PyMem_Free(data);
            return NULL;
        }

        offset = 1;
    }
    else if (dim == 2)
    {
        choose_ls_tuplets(lss, &ls_triplets, count);
    }

    data->ls_out = count;

    for (i = 0; i < count; i++)
    {
        data->out_patches[i] = outputPatches[i];
    }

    data->dimension = dim;
    data->ls_am = maxOutputPatch;

    for (i = 0; i < MAX_LS_AMOUNT; i++)
    {
        data->gains[i] = data->y[i] = 0.0;
    }

    i = 0;
    ls_ptr = ls_triplets;

    while (ls_ptr != NULL)
    {
        ls_ptr = ls_ptr->next;
        i++;
    }

    data->ls_set_am = i;
    data->ls_sets = (LS_SET *)PyMem_Malloc(sizeof(LS_SET) * i);

    i = 0;
    ls_ptr = ls_triplets;

    while (ls_ptr != NULL)
    {
        for (j = 0; j < data->dimension; j++)
        {
            //data->ls_sets[i].ls_nos[j] = ls_ptr->ls_nos[j] + offset;
            data->ls_sets[i].ls_nos[j] = outputPatches[ls_ptr->ls_nos[j] + offset - 1];
        }

        for (j = 0; j < (data->dimension * data->dimension); j++)
        {
            data->ls_sets[i].inv_mx[j] = ls_ptr->inv_mx[j];
        }

        ls_ptr = ls_ptr->next;
        i++;
    }

    free_ls_triplet_chain(ls_triplets);

    return data;
}

VBAP_DATA * copy_vbap_data(VBAP_DATA *data)
{
    int i, j;
    VBAP_DATA *nw = (VBAP_DATA *)PyMem_Malloc(sizeof(VBAP_DATA));
    nw->dimension = data->dimension;
    nw->ls_out = data->ls_out;

    for (i = 0; i < data->ls_out; i++)
    {
        nw->out_patches[i] = data->out_patches[i];
    }

    nw->ls_am = data->ls_am;
    nw->ls_set_am = data->ls_set_am;

    for (i = 0; i < MAX_LS_AMOUNT; i++)
    {
        nw->gains[i] = data->gains[i];
        nw->y[i] = data->y[i];
    }

    nw->ls_sets = (LS_SET *)PyMem_Malloc(sizeof(LS_SET) * nw->ls_set_am);

    for (i = 0; i < nw->ls_set_am; i++)
    {
        for (j = 0; j < nw->dimension; j++)
        {
            nw->ls_sets[i].ls_nos[j] = data->ls_sets[i].ls_nos[j];
        }

        for (j = 0; j < nw->dimension * nw->dimension; j++)
        {
            nw->ls_sets[i].inv_mx[j] = data->ls_sets[i].inv_mx[j];
        }
    }

    nw->ang_dir.azi = data->ang_dir.azi;
    nw->ang_dir.ele = data->ang_dir.ele;
    nw->ang_dir.length = data->ang_dir.length;
    nw->cart_dir.x = data->cart_dir.x;
    nw->cart_dir.y = data->cart_dir.y;
    nw->cart_dir.z = data->cart_dir.z;
    nw->spread_base.x = data->spread_base.x;
    nw->spread_base.y = data->spread_base.y;
    nw->spread_base.z = data->spread_base.z;
    return nw;
}

void free_vbap_data(VBAP_DATA *data)
{
    PyMem_Free(data->ls_sets);
    PyMem_Free(data);
}

void vbap(float azi, float ele, float spread, VBAP_DATA *data)
{
    int i;
    data->ang_dir.azi = azi;
    data->ang_dir.ele = ele;
    data->ang_dir.length = 1.0;
    vec_angle_to_cart(&data->ang_dir, &data->cart_dir);
    data->spread_base.x = data->cart_dir.x;
    data->spread_base.y = data->cart_dir.y;
    data->spread_base.z = data->cart_dir.z;

    for (i = 0; i < data->ls_am; i++)
    {
        data->gains[i] = 0.0;
    }

    compute_gains(data->ls_set_am, data->ls_sets, data->gains,
                  data->ls_am, data->cart_dir, data->dimension);

    if (spread > 0)
    {
        spreadit(azi, spread, data);
    }
}

void vbap2(float azi, float ele, float sp_azi,
           float sp_ele, VBAP_DATA *data)
{
    int i;
    data->ang_dir.azi = azi;
    data->ang_dir.ele = ele;
    data->ang_dir.length = 1.0;
    vec_angle_to_cart(&data->ang_dir, &data->cart_dir);

    for (i = 0; i < data->ls_am; i++)
    {
        data->gains[i] = 0.0;
    }

    compute_gains(data->ls_set_am, data->ls_sets, data->gains,
                  data->ls_am, data->cart_dir, data->dimension);

    if (data->dimension == 3)
    {
        if (sp_azi > 0 || sp_ele > 0)
        {
            spreadit_azi_ele(azi, ele, sp_azi, sp_ele, data);
        }
    }
    else
    {
        if (sp_azi > 0)
        {
            spreadit_azi(azi, sp_azi, data);
        }
    }
}

void vbap_flip_y_z(float azi, float ele, float spread, VBAP_DATA *data)
{
    int i;
    float tmp;
    data->ang_dir.azi = azi;
    data->ang_dir.ele = ele;
    data->ang_dir.length = 1.0;
    vec_angle_to_cart(&data->ang_dir, &data->cart_dir);
    tmp = data->cart_dir.z;
    data->cart_dir.z = data->cart_dir.y;
    data->cart_dir.y = tmp;
    data->spread_base.x = data->cart_dir.x;
    data->spread_base.y = data->cart_dir.y;
    data->spread_base.z = data->cart_dir.z;

    for (i = 0; i < data->ls_am; i++)
    {
        data->gains[i] = 0.0;
    }

    compute_gains(data->ls_set_am, data->ls_sets, data->gains,
                  data->ls_am, data->cart_dir, data->dimension);

    if (spread > 0)
    {
        spreadit(azi, spread, data);
    }
}

void vbap2_flip_y_z(float azi, float ele, float sp_azi,
                    float sp_ele, VBAP_DATA *data)
{
    int i;
    float tmp;
    data->ang_dir.azi = azi;
    data->ang_dir.ele = ele;
    data->ang_dir.length = 1.0;
    vec_angle_to_cart(&data->ang_dir, &data->cart_dir);
    tmp = data->cart_dir.z;
    data->cart_dir.z = data->cart_dir.y;
    data->cart_dir.y = tmp;

    for (i = 0; i < data->ls_am; i++)
    {
        data->gains[i] = 0.0;
    }

    compute_gains(data->ls_set_am, data->ls_sets, data->gains,
                  data->ls_am, data->cart_dir, data->dimension);

    if (data->dimension == 3)
    {
        if (sp_azi > 0 || sp_ele > 0)
        {
            spreadit_azi_ele_flip_y_z(azi, ele, sp_azi, sp_ele, data);
        }
    }
    else
    {
        if (sp_azi > 0)
        {
            spreadit_azi_flip_y_z(azi, sp_azi, data);
        }
    }
}

/* Selects a vector base of a virtual source.
 * Calculates gain factors in that base. */
void compute_gains(int ls_set_am, LS_SET *sets, float *gains,
                   int ls_amount, CART_VEC cart_dir, int dim)
{
    int i, j, k, tmp2;
    float vec[3], tmp;
    /* Direction of the virtual source in cartesian coordinates. */
    vec[0] = cart_dir.x;
    vec[1] = cart_dir.y;
    vec[2] = cart_dir.z;

    for (i = 0; i < ls_set_am; i++)
    {
        sets[i].set_gains[0] = 0.0;
        sets[i].set_gains[1] = 0.0;
        sets[i].set_gains[2] = 0.0;
        sets[i].smallest_wt  = 1000.0;
        sets[i].neg_g_am = 0;
    }

    for (i = 0; i < ls_set_am; i++)
    {
        for (j = 0; j < dim; j++)
        {
            for (k = 0; k < dim; k++)
            {
                sets[i].set_gains[j] += vec[k] * sets[i].inv_mx[((dim * j) + k)];
            }

            if (sets[i].smallest_wt > sets[i].set_gains[j])
                sets[i].smallest_wt = sets[i].set_gains[j];

            if (sets[i].set_gains[j] < -0.05)
                sets[i].neg_g_am++;
        }
    }

    j = 0;
    tmp = sets[0].smallest_wt;
    tmp2 = sets[0].neg_g_am;

    for (i = 1; i < ls_set_am; i++)
    {
        if (sets[i].neg_g_am < tmp2)
        {
            tmp = sets[i].smallest_wt;
            tmp2 = sets[i].neg_g_am;
            j = i;
        }
        else if (sets[i].neg_g_am == tmp2)
        {
            if (sets[i].smallest_wt > tmp)
            {
                tmp = sets[i].smallest_wt;
                tmp2 = sets[i].neg_g_am;
                j = i;
            }
        }
    }

    if (sets[j].set_gains[0] <= 0.0 &&
            sets[j].set_gains[1] <= 0.0 &&
            sets[j].set_gains[2] <= 0.0)
    {

        sets[j].set_gains[0] = 1.0;
        sets[j].set_gains[1] = 1.0;
        sets[j].set_gains[2] = 1.0;
    }

    memset(gains, 0, ls_amount * sizeof(float));

    gains[sets[j].ls_nos[0] - 1] = sets[j].set_gains[0];
    gains[sets[j].ls_nos[1] - 1] = sets[j].set_gains[1];

    if (dim == 3)
        gains[sets[j].ls_nos[2] - 1] = sets[j].set_gains[2];

    for (i = 0; i < ls_amount; i++)
    {
        if (gains[i] < 0.0)
            gains[i] = 0.0;
    }
}

int vbap_get_triplets(VBAP_DATA *data, int ***triplets)
{
    int i, num = data->ls_set_am;
    (*triplets) = (int **)PyMem_Malloc(num * sizeof(int *));

    for (i = 0; i < num; i++)
    {
        (*triplets)[i] = (int *)PyMem_Malloc(3 * sizeof(int));
        (*triplets)[i][0] = data->ls_sets[i].ls_nos[0];
        (*triplets)[i][1] = data->ls_sets[i].ls_nos[1];
        (*triplets)[i][2] = data->ls_sets[i].ls_nos[2];
    }

    return num;
}
