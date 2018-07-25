/* 
  This program was tested with GCC 5.4.0 on Ubuntu 16.04
  and has backwards compatibility with C89 (flag -std=c89).
  No compiler specific directives or architecture intrinsics
  were used.
  Compile with: gcc -Wall radix.c -o radix 
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int array_sorted(int vector[], int size);
void int_radix_sort(register int vector[], register unsigned int size);

/* Small functionality test */
int main(int argc, char * argv []) {

    /* Initial variable declaration */
    srand(time(NULL));
    clock_t t;
    int size = 100000000;
    int num_max = 2147483647;
    int num_min = 0;
    int i;

    /* User input */
    printf("Enter the number of elements: ");
    scanf("%d", &size);
    printf("Enter the minimun number: ");
    scanf("%d", &num_min);
    printf("Enter the maximun number: ");
    scanf("%d", &num_max);

    /* Check for inconsistensies and constrains */
    if(num_min > num_max) {
	int temp = num_min;
	num_min = num_max;
	num_max = temp;
    }
    if(size < 0) {
	size = -size;
    }

    /* Allocate and randomly shuffle the array */
    int *a = (int *)malloc(sizeof(int) * size);
    for(i = 0; i < size; ++i) {
	a[i] = (rand() % (num_max - num_min)) + num_min;
    }
        
    /* Time sort function execution */
    t = clock();
    int_radix_sort(a, size);
    t = clock() - t;
    double time = (double)t / CLOCKS_PER_SEC;
  
    /* Print results */
    printf("\nRadix sort took %f seconds.\n[sorted %d numbers from %d to %d].\n",
	   time, size, num_min, num_max);

    if(array_sorted(a, size) != 0){
	printf("The array was sorted successfully!\n");
    } else {
    	printf("The array wasn't fully sorted. Please report this problem!\n");
    }

    free(a);
    
    return 0;
}

/* Sanity check for unsorted elements */
int array_sorted(int vector[], int size) {
    int i, flag = 1;
    for(i = 1; i < size && flag; ++i) {
	if(vector[i] < vector[i - 1]) {
	    flag = 0;
	}
    }
    return flag;
}

/*
  Integer Radix LSD sort. Stable and out-of-place.
  ---Parameters---
  
  vector[] - Pointer to the orginal array of integers
  size     - Size of the array to sort
 
 ---List of optimizations implemented---
  For a list of all optimizations implemented check the github README.md
  over at https://github.com/AwardOfSky/Fast-Radix-Sort
 */
void int_radix_sort(register int vector[], register int size) {

    /* Support for variable sized integers without overflow warnings */
#define MAX_UINT__ ((unsigned int)(~0) >> 1)
#define LAST_EXP__ (sizeof(int) << 3)
    /* Define std preliminary, abosulte max value and if there are bytes left */
#define PRELIMINARY__ 100
#define ABS_MAX__ ((max < -exp) ? -exp : max)
#define MISSING_BITS__ exp < LAST_EXP__ && (max >> exp) > 0
    /* Check for biggest integer in [a, b[ array segment */
#define LOOP_MAX__(a, b)				\
    for(s = &vector[a], k = &vector[b]; s < k; ++s) {	\
	if(*s > max || *s < exp) {			\
	    if(*s > max)  {				\
		max = *s;				\
	    } else{					\
		exp = *s;				\
	    }						\
	}						\
    }
    
    register int *helper; /* Helper array */
    register int *s, *k, i, j; /* Array iterators */
    register int exp = *vector; /* Bits sorted */
    register int max = exp;  /* Maximun range in array */
    register unsigned char *n, *m; /* Iterator of a byte within an integer */
    int swap = 0; /* Tells where sorted array is (if, sorted is in vector) */
    int last_byte_sorted = 0; /* 1 if we had to sort the last byte */
    int next = 0;  /* If 1 we skip the byte (all buckets are the same) */
    unsigned int init_size = size; /* Copy (needed if doing subdivisions) */

    /* Preliminary index value to retrieve some initial info from the array */
    const int preliminary = (size > PRELIMINARY__) ? PRELIMINARY__ : (size >> 3);
    
    /* Get max value (to know how many bytes to sort) */
    LOOP_MAX__(1, preliminary);
    if(ABS_MAX__ <= (MAX_UINT__ >> 7) || (max - exp == 0)) {
	LOOP_MAX__(preliminary, size);
    }
    unsigned int diff = max - exp;
    max = ABS_MAX__;

    /* Set number of bytes to sort according to max */
    int bytes_to_sort = 0;
    if(diff != 0) {
	bytes_to_sort = 1;
	exp = 8;
	while(exp < (sizeof(int) << 3) && (max >> (exp - 1)) > 0) {
	    bytes_to_sort++;
	    exp += 8;
	}
    } else { /* 1 unique element */
	return;
    }
    
    /* Helper array initialization */
    helper = (int *)malloc(sizeof(int) * size);

    /* Core algorithm template:                                              */
    /* 1 - Fill the buckets (using byte iterators)                           */
    /* 2 - Check if there is 1 bucket w/ all elements (no need to sort byte) */
    /* 3 - Set the pointers to the helper array if setting the byte          */
    /* 4 - Iterate the bucket values within the orginal array and chnage the */
    /*     corresponding values in the helper                                */
#define SORT_BYTE__(vector, helper, shift, bucket,			\
		    pointer, ptr_init, optional_ptr_init)		\
    int bucket[0x100] = {0};						\
    n = (unsigned char *)(vector) + (exp >> 3);				\
    for(m = (unsigned char *)(&vector[size & (~0 << 3)]); n < m;) {	\
	++bucket[*n]; n += sizeof(int);	++bucket[*n]; n += sizeof(int);	\
	++bucket[*n]; n += sizeof(int);	++bucket[*n]; n += sizeof(int);	\
	++bucket[*n]; n += sizeof(int);	++bucket[*n]; n += sizeof(int);	\
	++bucket[*n]; n += sizeof(int);	++bucket[*n]; n += sizeof(int);	\
    }									\
    for(n = (unsigned char *)(&vector[size & (~0 << 3)]) + (exp >> 3),	\
	    m = (unsigned char *)(&vector[size]); n < m;) {		\
	++bucket[*n]; n += sizeof(int);					\
    }									\
    s = helper;								\
    next = 0;								\
    if(size > 65535) {							\
	for(i = 0; i < 0x100 && !next; ++i) {				\
	    if(bucket[i] == size) {					\
		optional_ptr_init;					\
		next = 1;						\
	    }								\
	}								\
    }									\
    if(!next) {								\
	if(exp != (LAST_EXP__ - 8)) {					\
	    for(i = 0; i < 0x100; s += bucket[i++]) {			\
		ptr_init;						\
	    }								\
	} else {							\
	    for(i = 128; i < 0x100; s += bucket[i++]) {			\
		ptr_init;						\
	    }								\
	    for(i = 0; i < 128; s += bucket[i++]) {			\
		ptr_init;						\
	    }								\
	}								\
	for(s = vector, k = &vector[size]; s < k; ++s) {		\
	    *pointer[(*s shift) & 0xFF]++ = *s;				\
	}								\
	swap = 1 - swap;						\
    }									\
    exp += 8;

    int *point[0x100] = {0}; /* Array of pointers to the helper array */

    if(bytes_to_sort > 1 && size > 1600000) { /* MSB order (size > 1.6M) */

	exp -= 8;
	
	/* old_point will serve as a copy of the initial values of "point" */
	/* Beggining of each subarray in 1st subdivision (256 subarrays) */
	int *old_point[0x100] = {0};

	/* Sort last byte */
	SORT_BYTE__(vector, helper, >> exp, bucket, point,
		    old_point[i] = point[i] = s,
		    old_point[i] = s; point[i] = old_point[i] + size);
	bytes_to_sort--;
	if(exp == LAST_EXP__) {
	    last_byte_sorted = 1;
	}
	
	/* 2nd subdivision only for 3 bytes or more (and size > 512M) */
	if(bytes_to_sort > 1 && size > 512000000) {

	    exp -= 16;
	    
	    /* Same purpose as "point" and old_point" but for 2nd subdivision */
	    int *point_2msb[0x10000] = {0};
	    int *old_point_2msb[0x10000] = {0};
	    int swap_copy = swap; /* Reset value of swap after each subarray */

	    /* Sort second to last byte in LSB order (256 subdivisions) */
	    for(j = 0; j < 0x100; ++j) {
		size = point[j] - old_point[j];
		swap = swap_copy;

		/* Define temporary vector and helper according to current swap*/
		register int *sub_help, *sub_vec;
		if(swap) {
		    sub_vec = old_point[j];
		    sub_help = vector + (old_point[j] - helper);
		} else {
		    sub_vec = vector + (old_point[j] - helper);
		    sub_help = old_point[j];
		}
		
		/* Temporary for ea subdivision, these work as "array riders" */
		int **point_2msb_rider = point_2msb + (j << 8);
		int **old_point_2msb_rider = old_point_2msb + (j << 8);

		/* Actually sort the byte */
		SORT_BYTE__(sub_vec, sub_help, >> exp, bucket1, point_2msb_rider,
			    point_2msb_rider[i] = old_point_2msb_rider[i] = s,
			    old_point_2msb_rider[i] = s;
			    point_2msb_rider[i] = old_point_2msb_rider[i] +size);
		exp -= 8;
		
		/* Make sure the sorted array is in the original vector */
		if(swap) {
		    if(swap_copy) {
			memcpy(sub_help, sub_vec, sizeof(int) * size);
		    } else {
			memcpy(sub_vec, sub_help, sizeof(int) * size);
		    }
		}
			       
	    }
	    swap = 0; /* Because now sorted array is in vector*/
	    bytes_to_sort--;

	    /* Sort remaining bytes in LSB order (65536 subdivisions) */
	    max = 1 << ((bytes_to_sort - 1) << 3);
	    for(j = 0; j < 0x10000; ++j) {
		
		exp = 0;
		size = point_2msb[j] - old_point_2msb[j];
		swap = 0; /* Reset swap (last swap is always 0) */

		/* Define temp arrays according to wether the first MSB byte */
		/* was sorted or not (array pointed by old_point_2msb changes) */
		register int *sub_help, *sub_vec;
		if(swap_copy) {
		    sub_vec = old_point_2msb[j];
		    sub_help = helper + (old_point_2msb[j] - vector);
		} else {
		    sub_vec = vector + (old_point_2msb[j] - helper);
		    sub_help = old_point_2msb[j];
		}

		while(MISSING_BITS__) { /* While there are remaining bytes */
		    if(exp) {
			if(swap) {
			    SORT_BYTE__(sub_help, sub_vec, >> exp, bucket1,
					point, point[i] = s, );
			} else { /* Note: won't happen in 32 bit integers */
			    SORT_BYTE__(sub_vec, sub_help, >> exp, bucket1,
					point, point[i] = s, );
			}
		    } else {
			SORT_BYTE__(sub_vec, sub_help, , bucket1,
				    point, point[i] = s, );
		    }
		}

		if(swap) { /* Force sorted segments to be in original vector */
		    memcpy(sub_vec, sub_help, sizeof(int) * size);
		}

	    }
	    swap = 0;

	} else {

	    /* Start sorting from LSB now */
	    max = 1 << ((bytes_to_sort - 1) << 3);
	    int swap_copy = swap; /* Once more, reset swap in ea subarray */
	    for(j = 0; j < 0x100; ++j) {
	    	exp = 0;
	    	size = point[j] - old_point[j];
	    	swap = swap_copy;

		register int *sub_help, *sub_vec; /* Temprary arrays */
		if(swap) {
		    sub_help = vector + (old_point[j] - helper);
		    sub_vec = old_point[j];
		} else {
		    sub_help = old_point[j];
		    sub_vec = vector + (old_point[j] - helper);
		}

		int *temp_point[0x100]; /* Temp ptrs, just to sort this segment */
		while(MISSING_BITS__) { /* While there are remaining bytes */
		    if(exp) {
			if(swap != swap_copy) {
			    SORT_BYTE__(sub_help, sub_vec, >> exp, bucket1,
					temp_point, temp_point[i] = s, );
			} else {
			    SORT_BYTE__(sub_vec, sub_help, >> exp, bucket1,
					temp_point, temp_point[i] = s, );
			}
		    } else {
			SORT_BYTE__(sub_vec, sub_help, , buckett,
				    temp_point, temp_point[i] = s, );
		    }
		}

		if(swap) { /* Again, make sure sorted array is the vector */
		    if(swap_copy) {
			memcpy(sub_help, sub_vec, sizeof(int) * size);
		    } else {
			memcpy(sub_vec, sub_help, sizeof(int) * size);
		    }
		}
	    }
	    swap = 0;
	}
   	
    } else if(bytes_to_sort > 0) { /* Use normal LSB radix (no subarrays) */

	exp = 0; /* Start at the first byte */

	max = 1 << ((bytes_to_sort - 1) << 3);
	while(MISSING_BITS__) { /* Sort until there are no bytes left */
	    if(exp) {
		if(swap) {
		    SORT_BYTE__(helper, vector, >> exp, bucket,
				point, point[i] = s, );
		} else {
		    SORT_BYTE__(vector, helper, >> exp, bucket,
				point, point[i] = s, );
		}
	    } else {
		SORT_BYTE__(vector, helper, , bucket, point,
			    point[i] = s, );
	    }

	    if(exp == LAST_EXP__) { /* Check if last byte was sorted */
	    	last_byte_sorted = 1;
	    }
	    
	}
	
    }

    size = init_size; /* Restore size */
    int *v = vector;  /* Temporary values for the vfector and helper arrays */
    int *h = helper;
    /* In case the array has both negative and positive integers, find the    */
    /* index of the first negative integer and put those numbers in the start */
    if(!last_byte_sorted && (((*v ^ v[size - 1]) < 0 && !swap) ||
			     ((*h ^ h[size - 1]) < 0 && swap))) {

	int offset = size - 1;
    	int tminusoff;

	/* If sorted array is in vector, use helper to re-order and vs */
    	if(!swap)  {
    	    for(s = v, k = &v[size]; s < k && *s >= 0; ++s) { }
    	    offset = s - v;

    	    tminusoff = size - offset;
	    
    	    if(offset < tminusoff) {
    	    	memcpy(h, v, sizeof(int) * offset);
    	    	memcpy(v, v + offset, sizeof(int) * tminusoff);
    	    	memcpy(v + tminusoff, h, sizeof(int) * offset);
    	    } else {
    	    	memcpy(h, v + offset, sizeof(int) * tminusoff);
    	    	memmove(v + tminusoff, v, sizeof(int) * offset);
    	    	memcpy(v, h, sizeof(int) * tminusoff);
    	    }
	} else {
	    for(s = h, k = &h[size]; s < k && *s >= 0; ++s) { }
    	    offset = s - h;

    	    tminusoff = size - offset;

    	    memcpy(v, h + offset, sizeof(int) * tminusoff);
    	    memcpy(v + tminusoff, h, sizeof(int) * (size - tminusoff));
    	}
    } else if(swap) {
    	memcpy(v, h, sizeof(int) * size);
    }

    /* Free helper array */
    free(helper);

}
