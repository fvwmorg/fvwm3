/* -*-c-*- */
/* Copyright (C) 2002  Olivier Chapuis */

#ifndef FPNG_H
#define FPNH_H

/* ---------------------------- included header files ---------------------- */

#include "PictureBase.h"

#if PngSupport
#include <png.h>
#else
#include <setjmp.h>
#endif

/* ---------------------------- global definitions ------------------------- */

/* ---------------------------- global macros ------------------------------ */

/* ---------------------------- type definitions --------------------------- */

#ifndef _ZLIB_H
typedef unsigned char  FzByte;
typedef unsigned int   FzuInt;
typedef unsigned long  FzuLong;
typedef FzByte  FzBytef;
#ifdef __STDC__
typedef void *Fzvoidpf;
typedef void *Fzvoidp;
#else
typedef FzByte *Fzvoidpf;
typedef FzByte *Fzvoidp;
#endif
typedef Fzvoidpf (*Fzalloc_func) (
#ifdef __STDC__
	Fzvoidpf opaque, FzuInt items, FzuInt size
#endif
	);
typedef Fzvoidp (*Fzfree_func)  (
#ifdef __STDC__
	Fzvoidpf opaque, Fzvoidpf address
#endif
	);
typedef struct Fz_stream_s {
	FzBytef    *next_in;
	FzuInt     avail_in;
	FzuLong    total_in;
	FzBytef    *next_out;
	FzuInt     avail_out;
	FzuLong    total_out;
	char     *msg;
	struct internal_state *state;
	Fzalloc_func zalloc;
	Fzfree_func  zfree;
	Fzvoidpf     opaque;
	int     data_type;
	FzuLong   adler;
	FzuLong   reserved;
} Fz_stream;
typedef Fz_stream *Fz_streamp;
#else /* _ZLIB_H */
#ifdef Z_PREFIX
typedef z_Byte   FzByte;
typedef z_uInt   FzuInt;
typedef z_uLong  FzuLong;
typedef z_Bytef  FzBytef;
typedef z_voidp  Fzvoidp;
typedef z_voidpf Fzvoidpf;
typedef z_stream Fz_stream;
#else
typedef Byte   FzByte;
typedef uInt   FzuInt;
typedef uLong  FzuLong;
typedef Bytef  FzBytef;
typedef voidp  Fzvoidp;
typedef voidpf Fzvoidpf;
typedef z_stream Fz_stream;
#endif
#endif /* !_ZLIB_H */

#if PngSupport
typedef png_uint_32 Fpng_uint_32;
typedef png_int_32 Fpng_int_32;
typedef png_uint_16 Fpng_uint_16;
typedef png_int_16 Fpng_int_16;
typedef png_byte Fpng_byte;
typedef png_size_t Fpng_size_t;

typedef png_struct Fpng_struct;
typedef png_structp Fpng_structp;
typedef png_structpp Fpng_structpp;
typedef png_info Fpng_info;
typedef png_infop Fpng_infop;
typedef png_infopp Fpng_infopp;
#else
typedef unsigned long Fpng_uint_32;
typedef long Fpng_int_32;
typedef unsigned short Fpng_uint_16;
typedef short Fpng_int_16;
typedef unsigned char Fpng_byte;
typedef size_t Fpng_size_t;
typedef void             *Fpng_voidp;
typedef Fpng_byte        *Fpng_bytep;
typedef Fpng_uint_32     *Fpng_uint_32p;
typedef Fpng_int_32      *Fpng_int_32p;
typedef Fpng_uint_16     *Fpng_uint_16p;
typedef Fpng_int_16      *Fpng_int_16p;
typedef const char       *Fpng_const_charp;
typedef char             *Fpng_charp;
typedef double           *Fpng_doublep;
typedef Fpng_byte         **Fpng_bytepp;
typedef Fpng_uint_32      **Fpng_uint_32pp;
typedef Fpng_int_32       **Fpng_int_32pp;
typedef Fpng_uint_16      **Fpng_uint_16pp;
typedef Fpng_int_16       **Fpng_int_16pp;
typedef const char   **Fpng_const_charpp;
typedef char             **Fpng_charpp;
typedef double           **Fpng_doublepp;
typedef char             ** *Fpng_charppp;
typedef struct Fpng_color_struct
{
	Fpng_byte red;
	Fpng_byte green;
	Fpng_byte blue;
} Fpng_color;
typedef Fpng_color  *Fpng_colorp;
typedef Fpng_color  **Fpng_colorpp;
typedef struct Fpng_color_16_struct
{
	Fpng_byte index;
	Fpng_uint_16 red;
	Fpng_uint_16 green;
	Fpng_uint_16 blue;
	Fpng_uint_16 gray;
} Fpng_color_16;
typedef Fpng_color_16  *Fpng_color_16p;
typedef Fpng_color_16  **Fpng_color_16pp;
typedef struct Fpng_color_8_struct
{
	Fpng_byte red;
	Fpng_byte green;
	Fpng_byte blue;
	Fpng_byte gray;
	Fpng_byte alpha;
} Fpng_color_8;
typedef Fpng_color_8  *Fpng_color_8p;
typedef Fpng_color_8  **Fpng_color_8pp;
typedef struct Fpng_text_struct
{
	int compression;
	Fpng_charp key;
	Fpng_charp text;
	Fpng_size_t text_length;
} Fpng_text;
typedef Fpng_text  *Fpng_textp;
typedef Fpng_text  **Fpng_textpp;
typedef struct Fpng_time_struct
{
	Fpng_uint_16 year;
	Fpng_byte month;
	Fpng_byte day;
	Fpng_byte hour;
	Fpng_byte minute;
	Fpng_byte second;
} Fpng_time;
typedef Fpng_time  *Fpng_timep;
typedef Fpng_time  **Fpng_timepp;
typedef struct Fpng_info_struct
{
	Fpng_uint_32 width;
	Fpng_uint_32 height;
	Fpng_uint_32 valid;
	Fpng_uint_32 rowbytes;
	Fpng_colorp palette;
	Fpng_uint_16 num_palette;
	Fpng_uint_16 num_trans;
	Fpng_byte bit_depth;
	Fpng_byte color_type;
	Fpng_byte compression_type;
	Fpng_byte filter_type;
	Fpng_byte interlace_type;
	Fpng_byte channels;
	Fpng_byte pixel_depth;
	Fpng_byte spare_byte;
	Fpng_byte signature[8];
	float gamma;
	Fpng_byte srgb_intent;
	int num_text;
	int max_text;
	Fpng_textp text;
	Fpng_time mod_time;
	Fpng_color_8 sig_bit;
	Fpng_bytep trans;
	Fpng_color_16 trans_values;
	Fpng_color_16 background;
	Fpng_uint_32 x_offset;
	Fpng_uint_32 y_offset;
	Fpng_byte offset_unit_type;
	Fpng_uint_32 x_pixels_per_unit;
	Fpng_uint_32 y_pixels_per_unit;
	Fpng_byte phys_unit_type;
	Fpng_uint_16p hist;
	float x_white;
	float y_white;
	float x_red;
	float y_red;
	float x_green;
	float y_green;
	float x_blue;
	float y_blue;
	Fpng_charp pcal_purpose;
	Fpng_int_32 pcal_X0;
	Fpng_int_32 pcal_X1;
	Fpng_charp pcal_units;
	Fpng_charpp pcal_params;
	Fpng_byte pcal_type;
	Fpng_byte pcal_nparams;
} Fpng_info;
typedef Fpng_info  *Fpng_infop;
typedef Fpng_info  **Fpng_infopp;
typedef struct Fpng_struct_def Fpng_struct;
typedef Fpng_struct  *Fpng_structp;
typedef struct Fpng_row_info_struct
{
	Fpng_uint_32 width;
	Fpng_uint_32 rowbytes;
	Fpng_byte color_type;
	Fpng_byte bit_depth;
	Fpng_byte channels;
	Fpng_byte pixel_depth;
} Fpng_row_info;
typedef Fpng_row_info  *Fpng_row_infop;
typedef Fpng_row_info  **Fpng_row_infopp;
typedef void (*Fpng_error_ptr)(
#ifdef __STDC__
	Fpng_structp, Fpng_const_charp
#endif
	);
typedef void (*Fpng_rw_ptr)(
#ifdef __STDC__
	Fpng_structp, Fpng_bytep, Fpng_size_t
#endif
	);
typedef void (*Fpng_flush_ptr) (
#ifdef __STDC__
	Fpng_structp
#endif
	);
typedef void (*Fpng_read_status_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_uint_32, int
#endif
	);
typedef void (*Fpng_write_status_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_uint_32, int
#endif
	);
typedef void (*Fpng_progressive_info_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_infop
#endif
	);
typedef void (*Fpng_progressive_end_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_infop
#endif
	);
typedef void (*Fpng_progressive_row_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_bytep,Fpng_uint_32, int
#endif
	);
typedef void (*Fpng_user_transform_ptr)(
#ifdef __STDC__
	Fpng_structp, Fpng_row_infop, Fpng_bytep
#endif
	);
typedef Fpng_voidp (*Fpng_malloc_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_size_t
#endif
	);
typedef void (*Fpng_free_ptr) (
#ifdef __STDC__
	Fpng_structp, Fpng_voidp
#endif
	);
struct png_struct_def
{
	jmp_buf jmpbuf;
	Fpng_error_ptr error_fn;
	Fpng_error_ptr warning_fn;
	Fpng_voidp error_ptr;
	Fpng_rw_ptr write_data_fn;
	Fpng_rw_ptr read_data_fn;
	Fpng_voidp io_ptr;
	Fpng_user_transform_ptr read_user_transform_fn;
	Fpng_user_transform_ptr write_user_transform_fn;
	Fpng_voidp user_transform_ptr;
	Fpng_byte user_transform_depth;
	Fpng_byte user_transform_channels;
	Fpng_uint_32 mode;
	Fpng_uint_32 flags;
	Fpng_uint_32 transformations;
	Fz_stream zstream;
	Fpng_bytep zbuf;
	Fpng_size_t zbuf_size;
	int zlib_level;
	int zlib_method;
	int zlib_window_bits;
	int zlib_mem_level;
	int zlib_strategy;
	Fpng_uint_32 width;
	Fpng_uint_32 height;
	Fpng_uint_32 num_rows;
	Fpng_uint_32 usr_width;
	Fpng_uint_32 rowbytes;
	Fpng_uint_32 irowbytes;
	Fpng_uint_32 iwidth;
	Fpng_uint_32 row_number;
	Fpng_bytep prev_row;
	Fpng_bytep row_buf;
	Fpng_bytep sub_row;
	Fpng_bytep up_row;
	Fpng_bytep avg_row;
	Fpng_bytep paeth_row;
	Fpng_row_info row_info;
	Fpng_uint_32 idat_size;
	Fpng_uint_32 crc;
	Fpng_colorp palette;
	Fpng_uint_16 num_palette;
	Fpng_uint_16 num_trans;
	Fpng_byte chunk_name[5];
	Fpng_byte compression;
	Fpng_byte filter;
	Fpng_byte interlaced;
	Fpng_byte pass;
	Fpng_byte do_filter;
	Fpng_byte color_type;
	Fpng_byte bit_depth;
	Fpng_byte usr_bit_depth;
	Fpng_byte pixel_depth;
	Fpng_byte channels;
	Fpng_byte usr_channels;
	Fpng_byte sig_bytes;
	Fpng_uint_16 filler;
	Fpng_byte background_gamma_type;
	float background_gamma;
	Fpng_color_16 background;
	Fpng_color_16 background_1;
	Fpng_flush_ptr output_flush_fn;
	Fpng_uint_32 flush_dist;
	Fpng_uint_32 flush_rows;
	int gamma_shift;
	float gamma;
	float screen_gamma;
	Fpng_bytep gamma_table;
	Fpng_bytep gamma_from_1;
	Fpng_bytep gamma_to_1;
	Fpng_uint_16pp gamma_16_table;
	Fpng_uint_16pp gamma_16_from_1;
	Fpng_uint_16pp gamma_16_to_1;
	Fpng_color_8 sig_bit;
	Fpng_color_8 shift;
	Fpng_bytep trans;
	Fpng_color_16 trans_values;
	Fpng_read_status_ptr read_row_fn;
	Fpng_write_status_ptr write_row_fn;
	Fpng_progressive_info_ptr info_fn;
	Fpng_progressive_row_ptr row_fn;
	Fpng_progressive_end_ptr end_fn;
	Fpng_bytep save_buffer_ptr;
	Fpng_bytep save_buffer;
	Fpng_bytep current_buffer_ptr;
	Fpng_bytep current_buffer;
	Fpng_uint_32 push_length;
	Fpng_uint_32 skip_length;
	Fpng_size_t save_buffer_size;
	Fpng_size_t save_buffer_max;
	Fpng_size_t buffer_size;
	Fpng_size_t current_buffer_size;
	int process_mode;
	int cur_palette;
	Fpng_size_t current_text_size;
	Fpng_size_t current_text_left;
	Fpng_charp current_text;
	Fpng_charp current_text_ptr;
	Fpng_bytepp offset_table_ptr;
	Fpng_bytep offset_table;
	Fpng_uint_16 offset_table_number;
	Fpng_uint_16 offset_table_count;
	Fpng_uint_16 offset_table_count_free;
	Fpng_bytep palette_lookup;
	Fpng_bytep dither_index;
	Fpng_uint_16p hist;
	Fpng_byte heuristic_method;
	Fpng_byte num_prev_filters;
	Fpng_bytep prev_filters;
	Fpng_uint_16p filter_weights;
	Fpng_uint_16p inv_filter_weights;
	Fpng_uint_16p filter_costs;
	Fpng_uint_16p inv_filter_costs;
	Fpng_charp time_buffer;
	Fpng_voidp mem_ptr;
	Fpng_malloc_ptr malloc_fn;
	Fpng_free_ptr free_fn;
	Fpng_byte rgb_to_gray_status;
	Fpng_byte rgb_to_gray_red_coeff;
	Fpng_byte rgb_to_gray_green_coeff;
	Fpng_byte rgb_to_gray_blue_coeff;
	Fpng_byte empty_plte_permitted;
};
typedef Fpng_struct  **Fpng_structpp;
#endif
/* ---------------------------- global definitions ------------------------- */

#define FPNG_BYTES_TO_CHECK 4

#if PngSupport

#define FPNG_LIBPNG_VER_STRING PNG_LIBPNG_VER_STRING
#define FPNG_COLOR_TYPE_PALETTE PNG_COLOR_TYPE_PALETTE
#define FPNG_COLOR_TYPE_RGB_ALPHA PNG_COLOR_TYPE_RGB_ALPHA
#define FPNG_COLOR_TYPE_GRAY_ALPHA PNG_COLOR_TYPE_GRAY_ALPHA
#define FPNG_COLOR_TYPE_GRAY PNG_COLOR_TYPE_GRAY
#define FPNG_FILLER_BEFORE PNG_FILLER_BEFORE
#define FPNG_FILLER_AFTER PNG_FILLER_AFTER
#define FPNG_INFO_tRNS PNG_INFO_tRNS

#define Fpng_check_sig(a,b) png_check_sig(a,b)
#define Fpng_create_read_struct(a,b,c,d) png_create_read_struct(a,b,c,d)
#define Fpng_create_info_struct(a) png_create_info_struct(a)
#define Fpng_destroy_read_struct(a,b,c) png_destroy_read_struct(a,b,c)
#define Fpng_init_io(a,b) png_init_io(a,b)
#define Fpng_read_info(a,b) png_read_info(a,b)
#define Fpng_get_IHDR(a,b,c,d,e,f,g,h,i) png_get_IHDR(a,b,c,d,e,f,g,h,i)
#define Fpng_set_expand(a) png_set_expand(a)
#define Fpng_set_swap_alpha(a) png_set_swap_alpha(a)
#define Fpng_set_filler(a,b,c) png_set_filler(a,b,c)
#define Fpng_set_bgr(a) png_set_bgr(a)
#define Fpng_set_strip_16(a) png_set_strip_16(a)
#define Fpng_set_packing(a) png_set_packing(a)
#define Fpng_set_gray_to_rgb(a) png_set_gray_to_rgb(a)
#define Fpng_get_bit_depth(a,b) png_get_bit_depth(a,b)
#define Fpng_set_gray_1_2_4_to_8(a) png_set_gray_1_2_4_to_8(a)
#define Fpng_get_valid(a,b,c) png_get_valid(a,b,c)
#define Fpng_read_end(a,b) png_read_end(a,b)
#define Fpng_set_interlace_handling(a) png_set_interlace_handling(a)
#define Fpng_read_rows(a,b,c,d) png_read_rows(a,b,c,d)
#define Fpng_read_image(a,b) png_read_image(a,b)

#else

#define FPNG_LIBPNG_VER_STRING ""
#define FPNG_COLOR_TYPE_PALETTE 0
#define FPNG_COLOR_TYPE_RGB_ALPHA 1
#define FPNG_COLOR_TYPE_GRAY_ALPHA 2
#define FPNG_COLOR_TYPE_GRAY 3
#define FPNG_FILLER_BEFORE 6
#define FPNG_FILLER_AFTER 5
#define FPNG_INFO_tRNS 7

#define Fpng_check_sig(a,b) 0
#define Fpng_create_read_struct(a,b,c,d) NULL
#define Fpng_create_info_struct(a) NULL
#define Fpng_destroy_read_struct(a,b,c)
#define Fpng_init_io(a,b)
#define Fpng_read_info(a,b)
#define Fpng_get_IHDR(a,b,c,d,e,f,g,h,i)
#define Fpng_set_expand(a)
#define Fpng_set_swap_alpha(a)
#define Fpng_set_filler(a,b,c)
#define Fpng_set_bgr(a)
#define Fpng_set_strip_16(a)
#define Fpng_set_packing(a)
#define Fpng_set_gray_to_rgb(a)
#define Fpng_get_bit_depth(a,b) 0
#define Fpng_set_gray_1_2_4_to_8(a)
#define Fpng_get_valid(a,b,c) 0
#define Fpng_read_end(a,b)
#define Fpng_set_interlace_handling(a) 0
#define Fpng_read_rows(a,b,c,d)
#define Fpng_read_image(a,b)

#endif

#endif /* FPNG_H */
