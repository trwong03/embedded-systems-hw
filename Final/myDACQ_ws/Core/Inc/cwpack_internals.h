/*      CWPack - cwpack_internals.h   */
/*
 The MIT License (MIT)
 Copyright (c) 2017 Claes Wihlborg
 */

#ifndef CWPack_internals_H__
#define CWPack_internals_H__

#include "cwpack.h"

/*
 * DESIGN:
 *
 * cwpack.c's pack functions fall into two groups:
 *   A) Functions that declare "uint8_t *p;" at the top of the function:
 *      cw_pack_str, cw_pack_bin, cw_pack_ext, cw_pack_time, cw_pack_insert
 *   B) Functions that do NOT declare p:
 *      cw_pack_unsigned, cw_pack_signed, cw_pack_float, cw_pack_double,
 *      cw_pack_nil/true/false/boolean, cw_pack_array_size, cw_pack_map_size
 *
 * To handle both: cw_pack_reserve_space does NOT declare p.
 * It assigns pack_context->current to pack_context->current (bounds check
 * only), then tryMoveN macros write directly via pack_context->current
 * using a temporary pointer inside a GNU statement-expression ({ }).
 *
 * For functions in group A that use cw_pack_reserve_space outside a
 * tryMoveN (i.e. call it directly then use p), the macro assigns the
 * existing function-level p.
 *
 * Solution: use pack_context->current directly, rename the pointer
 * inside tryMoveN to _p to avoid any clash.
 *
 * Unpack functions always declare p, tmpu16, tmpu32, tmpu64 at top.
 */

/* =========================================================================
 * Byte-order helpers - used with function-level p in pack functions A,
 * and function-level p/tmp* in unpack functions.
 * ======================================================================= */

#define cw_store16(x) \
    p[0] = (uint8_t)((uint16_t)(x) >> 8); \
    p[1] = (uint8_t)(x);

#define cw_store32(x) \
    p[0] = (uint8_t)((uint32_t)(x) >> 24); \
    p[1] = (uint8_t)((uint32_t)(x) >> 16); \
    p[2] = (uint8_t)((uint32_t)(x) >>  8); \
    p[3] = (uint8_t)(x);

#define cw_store64(x) \
    p[0] = (uint8_t)((uint64_t)(x) >> 56); \
    p[1] = (uint8_t)((uint64_t)(x) >> 48); \
    p[2] = (uint8_t)((uint64_t)(x) >> 40); \
    p[3] = (uint8_t)((uint64_t)(x) >> 32); \
    p[4] = (uint8_t)((uint64_t)(x) >> 24); \
    p[5] = (uint8_t)((uint64_t)(x) >> 16); \
    p[6] = (uint8_t)((uint64_t)(x) >>  8); \
    p[7] = (uint8_t)(x);

#define cw_load16(ptr) \
    tmpu16 = (uint16_t)(((uint16_t)(ptr)[0] << 8) | (ptr)[1]); \
    (ptr) += 2;

#define cw_load32(ptr) \
    tmpu32 = (((uint32_t)(ptr)[0] << 24) | ((uint32_t)(ptr)[1] << 16) | \
              ((uint32_t)(ptr)[2] <<  8) |  (uint32_t)(ptr)[3]); \
    (ptr) += 4;

#define cw_load64(ptr, dst) \
    (dst) = (((uint64_t)(ptr)[0] << 56) | ((uint64_t)(ptr)[1] << 48) | \
             ((uint64_t)(ptr)[2] << 40) | ((uint64_t)(ptr)[3] << 32) | \
             ((uint64_t)(ptr)[4] << 24) | ((uint64_t)(ptr)[5] << 16) | \
             ((uint64_t)(ptr)[6] <<  8) |  (uint64_t)(ptr)[7]); \
    (ptr) += 8;

/* =========================================================================
 * PACK macros
 *
 * cw_pack_reserve_space: bounds-check only, then assigns the function-level
 * p (which exists in group-A functions).  For group-B functions, tryMoveN
 * uses its own private _p so p is never needed.
 *
 * tryMoveN: each opens its own block, uses a private _p, and returns.
 * This avoids any conflict with function-level p.
 * ======================================================================= */

/* Bounds-check + advance current. Assigns p for group-A functions.
 * In group-B functions p is unused - no warning because we never declare it. */
#define cw_pack_reserve_space(n) \
    if (pack_context->current + (n) > pack_context->end) { \
        if (!pack_context->handle_pack_overflow || \
            pack_context->handle_pack_overflow(pack_context, (unsigned long)(n)) != CWP_RC_OK) { \
            pack_context->return_code = CWP_RC_BUFFER_OVERFLOW; \
            return; \
        } \
    } \
    p = pack_context->current; \
    pack_context->current += (n);

/* tryMoveN: private _p, own block, no conflict with function-level p */
#define tryMove0(b) \
    { uint8_t *_p = pack_context->current; \
      if (_p + 1 > pack_context->end) { \
          if (!pack_context->handle_pack_overflow || \
              pack_context->handle_pack_overflow(pack_context, 1) != CWP_RC_OK) { \
              pack_context->return_code = CWP_RC_BUFFER_OVERFLOW; return; } \
          _p = pack_context->current; } \
      pack_context->current = _p + 1; \
      *_p = (uint8_t)(b); return; }

#define tryMove1(tag, val) \
    { uint8_t *_p = pack_context->current; \
      if (_p + 2 > pack_context->end) { \
          if (!pack_context->handle_pack_overflow || \
              pack_context->handle_pack_overflow(pack_context, 2) != CWP_RC_OK) { \
              pack_context->return_code = CWP_RC_BUFFER_OVERFLOW; return; } \
          _p = pack_context->current; } \
      pack_context->current = _p + 2; \
      _p[0] = (uint8_t)(tag); _p[1] = (uint8_t)(val); return; }

#define tryMove2(tag, val) \
    { uint8_t *_p = pack_context->current; \
      if (_p + 3 > pack_context->end) { \
          if (!pack_context->handle_pack_overflow || \
              pack_context->handle_pack_overflow(pack_context, 3) != CWP_RC_OK) { \
              pack_context->return_code = CWP_RC_BUFFER_OVERFLOW; return; } \
          _p = pack_context->current; } \
      pack_context->current = _p + 3; \
      _p[0] = (uint8_t)(tag); \
      _p[1] = (uint8_t)((uint32_t)(val) >> 8); \
      _p[2] = (uint8_t)(val); return; }

#define tryMove4(tag, val) \
    { uint8_t *_p = pack_context->current; \
      if (_p + 5 > pack_context->end) { \
          if (!pack_context->handle_pack_overflow || \
              pack_context->handle_pack_overflow(pack_context, 5) != CWP_RC_OK) { \
              pack_context->return_code = CWP_RC_BUFFER_OVERFLOW; return; } \
          _p = pack_context->current; } \
      pack_context->current = _p + 5; \
      _p[0] = (uint8_t)(tag); \
      _p[1] = (uint8_t)((uint32_t)(val) >> 24); \
      _p[2] = (uint8_t)((uint32_t)(val) >> 16); \
      _p[3] = (uint8_t)((uint32_t)(val) >>  8); \
      _p[4] = (uint8_t)(val); return; }

#define tryMove8(tag, val) \
    { uint8_t *_p = pack_context->current; \
      if (_p + 9 > pack_context->end) { \
          if (!pack_context->handle_pack_overflow || \
              pack_context->handle_pack_overflow(pack_context, 9) != CWP_RC_OK) { \
              pack_context->return_code = CWP_RC_BUFFER_OVERFLOW; return; } \
          _p = pack_context->current; } \
      pack_context->current = _p + 9; \
      _p[0] = (uint8_t)(tag); \
      _p[1] = (uint8_t)((uint64_t)(val) >> 56); \
      _p[2] = (uint8_t)((uint64_t)(val) >> 48); \
      _p[3] = (uint8_t)((uint64_t)(val) >> 40); \
      _p[4] = (uint8_t)((uint64_t)(val) >> 32); \
      _p[5] = (uint8_t)((uint64_t)(val) >> 24); \
      _p[6] = (uint8_t)((uint64_t)(val) >> 16); \
      _p[7] = (uint8_t)((uint64_t)(val) >>  8); \
      _p[8] = (uint8_t)(val); return; }

#define PACK_ERROR(code) \
    { pack_context->return_code = (code); return; }

/* =========================================================================
 * UNPACK macros - use function-level p, tmpu16, tmpu32, tmpu64
 * ======================================================================= */

#define cw_unpack_assert_space(n) \
    if (unpack_context->current + (n) > unpack_context->end) { \
        if (!unpack_context->handle_unpack_underflow || \
            unpack_context->handle_unpack_underflow(unpack_context, (unsigned long)(n)) != CWP_RC_OK) { \
            unpack_context->return_code = buffer_end_return_code; \
            return; \
        } \
    } \
    p = unpack_context->current; \
    unpack_context->current += (n);

#define cw_unpack_assert_space_sub(n, retval) \
    if (unpack_context->current + (n) > unpack_context->end) { \
        unpack_context->return_code = CWP_RC_BUFFER_UNDERFLOW; \
        return (retval); \
    } \
    p = unpack_context->current; \
    unpack_context->current += (n);

#define cw_unpack_assert_blob(field) \
    if (unpack_context->current + unpack_context->item.as.field.length > unpack_context->end) { \
        if (!unpack_context->handle_unpack_underflow || \
            unpack_context->handle_unpack_underflow(unpack_context, \
                unpack_context->item.as.field.length) != CWP_RC_OK) { \
            unpack_context->return_code = buffer_end_return_code; \
            return; \
        } \
    } \
    unpack_context->item.as.field.start = unpack_context->current; \
    unpack_context->current += unpack_context->item.as.field.length; \
    return;

#define getDDItem(itype, field, val) \
    unpack_context->item.type = (itype); \
    unpack_context->item.as.field = (val);

#define getDDItem1(itype, field, cast) \
    unpack_context->item.type = (itype); \
    cw_unpack_assert_space(1); \
    unpack_context->item.as.field = (cast)(*p);

#define getDDItem2(itype, field, cast) \
    unpack_context->item.type = (itype); \
    cw_unpack_assert_space(2); \
    unpack_context->item.as.field = (cast)(((uint16_t)p[0] << 8) | p[1]);

#define getDDItem4(itype, field, cast) \
    unpack_context->item.type = (itype); \
    cw_unpack_assert_space(4); \
    unpack_context->item.as.field = (cast)(((uint32_t)p[0] << 24) | \
                                           ((uint32_t)p[1] << 16) | \
                                           ((uint32_t)p[2] <<  8) | \
                                            (uint32_t)p[3]);

#define getDDItem8(itype) \
    unpack_context->item.type = (itype); \
    cw_unpack_assert_space(8); \
    cw_load64(p, tmpu64); \
    unpack_context->item.as.u64 = tmpu64;

#define getDDItemFix(n) \
    cw_unpack_assert_space(1 + (n)); \
    unpack_context->item.type = (cwpack_item_types)*(int8_t*)p; \
    unpack_context->item.as.ext.length = (n); \
    unpack_context->item.as.ext.start  = p + 1; \
    unpack_context->current = p + 1 + (n); \
    return;

#define UNPACK_ERROR(code) \
    { unpack_context->return_code = (code); return; }

#endif /* CWPack_internals_H__ */
