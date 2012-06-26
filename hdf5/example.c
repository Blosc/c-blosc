/*
    Copyright (C) 2010  Francesc Alted
    http://blosc.pytables.org
    License: MIT (see LICENSE.txt)

    Example program demonstrating use of the Blosc filter from C code.
    This is based on the LZF example (http://h5py.alfven.org) by
    Andrew Collette.

    To compile this program:

    h5cc [-DH5_USE_16_API] -msse2 ../blosc/*.c blosc_filter.c example.c \
         -o example -lpthread

    To run:

    $ ./example
    Blosc version info: 1.1.1 ($Date:: 2010-10-01 #$)
    Success!
    $ h5ls -v example.h5
    Opened "example.h5" with sec2 driver.
    dset                     Dataset {100/100, 100/100, 100/100}
        Location:  1:800
        Links:     1
        Chunks:    {1, 100, 100} 40000 bytes
        Storage:   4000000 logical bytes, 142097 allocated bytes, 2814.98% utilization
        Filter-0:  blosc-32001 OPT {1, 2, 4, 40000, 4, 1}
        Type:      native float

*/

#include <stdio.h>
#include "hdf5.h"
#include "blosc_filter.h"

#define SIZE 100*100*100
#define SHAPE {100,100,100}
#define CHUNKSHAPE {1,100,100}

int main(){

    static float data[SIZE];
    static float data_out[SIZE];
    const hsize_t shape[] = SHAPE;
    const hsize_t chunkshape[] = CHUNKSHAPE;
    char *version, *date;
    int r, i;
    unsigned int cd_values[6];
    int return_code = 1;

    hid_t fid, sid, dset, plist = 0;

    for(i=0; i<SIZE; i++){
        data[i] = i;
    }

    /* Register the filter with the library */
    r = register_blosc(&version, &date);
    printf("Blosc version info: %s (%s)\n", version, date);

    if(r<0) goto failed;

    sid = H5Screate_simple(3, shape, NULL);
    if(sid<0) goto failed;

    fid = H5Fcreate("example.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    if(fid<0) goto failed;

    plist = H5Pcreate(H5P_DATASET_CREATE);
    if(plist<0) goto failed;

    /* Chunked layout required for filters */
    r = H5Pset_chunk(plist, 3, chunkshape);
    if(r<0) goto failed;

    /* This is the easiest way to call Blosc with default values: 5
     for BloscLZ and shuffle active. */
    /* r = H5Pset_filter(plist, FILTER_BLOSC, H5Z_FLAG_OPTIONAL, 0, NULL);  */

    /* But you can also taylor Blosc parameters to your needs */
    /* 0 to 3 (inclusive) param slots are reserved. */
    cd_values[4] = 4;       /* compression level for BloscLZ */
    cd_values[5] = 1;       /* 0: shuffle not active, 1: shuffle active */

    /* Set the filter with 6 params */
    r = H5Pset_filter(plist, FILTER_BLOSC, H5Z_FLAG_OPTIONAL, 6, cd_values);

    if(r<0) goto failed;

#if H5_USE_16_API
    dset = H5Dcreate(fid, "dset", H5T_NATIVE_FLOAT, sid, plist);
#else
    dset = H5Dcreate(fid, "dset", H5T_NATIVE_FLOAT, sid, H5P_DEFAULT, plist, H5P_DEFAULT);
#endif
    if(dset<0) goto failed;

    r = H5Dwrite(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data);
    if(r<0) goto failed;

    r = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, &data_out);
    if(r<0) goto failed;

    for(i=0;i<SIZE;i++){
        if(data[i] != data_out[i]) goto failed;
    }

    fprintf(stdout, "Success!\n");

    return_code = 0;

    failed:

    if(dset>0)  H5Dclose(dset);
    if(sid>0)   H5Sclose(sid);
    if(plist>0) H5Pclose(plist);
    if(fid>0)   H5Fclose(fid);

    return return_code;
}
