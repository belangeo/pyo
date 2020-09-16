/*
 * Functions for 3D VBAP processing based on work by Ville Pulkki.
 * (c) Ville Pulkki - 2.2.1999 Helsinki University of Technology.
 * Updated by belangeo, 2017.
 */

#ifndef __VBAP_H
#define __VBAP_H

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LS_AMOUNT 256
#define MAX_TRIPLET_AMOUNT 128
#define MIN_VOL_P_SIDE_LGTH 0.01

typedef struct
{
    int dimension;      /* Number of dimension, always 3. */
    int count;          /* Number of speakers. */
    float *azimuth;     /* Azimuth angle of speakers. */
    float *elevation;   /* Elevation angle of speakers. */
} SPEAKERS_SETUP;

/* Cartesian vector for a speaker position. */
typedef struct
{
    float x;
    float y;
    float z;
} CART_VEC;

/* Angular vector for a speaker position. */
typedef struct
{
    float azi;
    float ele;
    float length;
} ANG_VEC;

/* A struct for a loudspeaker triplet or pair (set). */
typedef struct
{
    int ls_nos[3];
    float inv_mx[9];
    float set_gains[3];
    float smallest_wt;
    int neg_g_am;
} LS_SET;

/* A struct for a loudspeaker instance. */
typedef struct
{
    CART_VEC coords;
    ANG_VEC angles;
} ls; // TODO: rename this struct.

/* VBAP structure of n loudspeaker panning */
typedef struct
{
    int out_patches[MAX_LS_AMOUNT];     /* Physical outputs (starts at 1). */
    float gains[MAX_LS_AMOUNT];         /* Loudspeaker gains. */
    float y[MAX_LS_AMOUNT];             /* Loudspeaker gains smoothing. */
    int dimension;                      /* Dimensions, 2 or 3. */
    LS_SET *ls_sets;                    /* Loudspeaker triplet structure. */
    int ls_out;                         /* Number of output patches. */
    int ls_am;                          /* Number of loudspeakers. */
    int ls_set_am;                      /* Number of triplets. */
    ANG_VEC ang_dir;                    /* Angular direction. */
    CART_VEC cart_dir;                  /* Cartesian direction. */
    CART_VEC spread_base;               /* Spreading vector. */
} VBAP_DATA;

/* Fill a SPEAKERS_SETUP structure from values.
 */
SPEAKERS_SETUP * load_speakers_setup(int cnt, float *azi, float *ele);

/* Fill a SPEAKERS_SETUP structure from the content of a text file.
 *
 * File format:
 *
 * First line starts with an integer which is the number of speakers.
 * Remaining lines (must be equal to the number of speakers) starts with
 * two floats. They give the azimuth and elevation for each speakers.
 */
SPEAKERS_SETUP * load_speakers_setup_from_file(const char *filename);

/* Properly free a previously allocated SPEAKERS_SETUP structure.
 */
void free_speakers_setup(SPEAKERS_SETUP *setup);

/* Initialize a VBAP_DATA structure from a loudspeakers setup and
 * an optional matrix of user-defined triplets.
 */
VBAP_DATA * init_vbap_data(SPEAKERS_SETUP *setup, int **triplets);

VBAP_DATA * init_vbap_from_speakers(ls lss[MAX_LS_AMOUNT], int count,
                                    int dim, int outputPatches[MAX_LS_AMOUNT],
                                    int maxOutputPatch, int **triplets);

VBAP_DATA * copy_vbap_data(VBAP_DATA *data);

/* Properly free a previously allocated VBAP_DATA structure.
 */
void free_vbap_data(VBAP_DATA *data);

/* Calculates gain factors using loudspeaker setup and angle direction.
 */
void vbap(float azi, float ele, float spread, VBAP_DATA *data);
void vbap2(float azi, float ele, float sp_azi,
           float sp_ele, VBAP_DATA *data);
void vbap_flip_y_z(float azi, float ele, float spread, VBAP_DATA *data);
void vbap2_flip_y_z(float azi, float ele, float sp_azi,
                    float sp_ele, VBAP_DATA *data);

int vbap_get_triplets(VBAP_DATA *data, int ***triplets);

#ifdef __cplusplus
}
#endif

#endif /* __VBAP_H */

