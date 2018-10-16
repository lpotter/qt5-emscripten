/*
 * Copyright (C) 2015 The Qt Company Ltd
 *
 * This is part of HarfBuzz, an OpenType Layout engine library.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef HARFBUZZ_EXTERNAL_H
#define HARFBUZZ_EXTERNAL_H

#include "harfbuzz-global.h"

HB_BEGIN_HEADER

/* This header contains some methods that are not part of
   Harfbuzz itself, but referenced by it.
   They need to be provided by the application/library
*/

typedef enum
{
    HB_Mark_NonSpacing,          /*   Mn */
    HB_Mark_SpacingCombining,    /*   Mc */
    HB_Mark_Enclosing,           /*   Me */

    HB_Number_DecimalDigit,      /*   Nd */
    HB_Number_Letter,            /*   Nl */
    HB_Number_Other,             /*   No */

    HB_Separator_Space,          /*   Zs */
    HB_Separator_Line,           /*   Zl */
    HB_Separator_Paragraph,      /*   Zp */

    HB_Other_Control,            /*   Cc */
    HB_Other_Format,             /*   Cf */
    HB_Other_Surrogate,          /*   Cs */
    HB_Other_PrivateUse,         /*   Co */
    HB_Other_NotAssigned,        /*   Cn */

    HB_Letter_Uppercase,         /*   Lu */
    HB_Letter_Lowercase,         /*   Ll */
    HB_Letter_Titlecase,         /*   Lt */
    HB_Letter_Modifier,          /*   Lm */
    HB_Letter_Other,             /*   Lo */

    HB_Punctuation_Connector,    /*   Pc */
    HB_Punctuation_Dash,         /*   Pd */
    HB_Punctuation_Open,         /*   Ps */
    HB_Punctuation_Close,        /*   Pe */
    HB_Punctuation_InitialQuote, /*   Pi */
    HB_Punctuation_FinalQuote,   /*   Pf */
    HB_Punctuation_Other,        /*   Po */

    HB_Symbol_Math,              /*   Sm */
    HB_Symbol_Currency,          /*   Sc */
    HB_Symbol_Modifier,          /*   Sk */
    HB_Symbol_Other              /*   So */
} HB_CharCategory;

void HB_GetUnicodeCharProperties(HB_UChar32 ch, HB_CharCategory *category, int *combiningClass);
HB_CharCategory HB_GetUnicodeCharCategory(HB_UChar32 ch);
int HB_GetUnicodeCharCombiningClass(HB_UChar32 ch);
HB_UChar16 HB_GetMirroredChar(HB_UChar16 ch);

void (*HB_Library_Resolve(const char *library, int version, const char *symbol))();

HB_END_HEADER

#endif
