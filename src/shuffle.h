// Shuffle/unshuffle routines

void shuffle(size_t bytesoftype, size_t blocksize,
             unsigned char* _src, unsigned char* _dest);

void unshuffle(size_t bytesoftype, size_t blocksize,
               unsigned char* _src, unsigned char* _dest);
