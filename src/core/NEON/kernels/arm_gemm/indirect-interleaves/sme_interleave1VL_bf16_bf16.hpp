/*
 * Copyright (c) 2022-2024 Arm Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if defined(ARM_COMPUTE_ENABLE_SME)

template <>
void interleave_block<1, 1, VLType::SME, false>(
  bfloat16 * &out, const bfloat16 * const *in,
  size_t width, size_t height, size_t row_offset, bool
)
{
  __asm__ __volatile__(
      ".inst 0xd503477f  // SMSTART ZA\n"
      "mov x21, %x[width]\n"
      "mov x20, %x[width]\n"
      "inch x21\n"
      "cnth x11\n"
      "sub x21, x21, #0x1\n"
      "sub x10, x11, #0x1\n"
      "udiv x21, x21, x11\n"  // n_passes = ceildiv(width, VL<T>)
      "ands x10, x20, x10\n"
      "sub x9, x21, #0x1\n"
      "sub x28, x11, #0x2\n"
      "lsl x20, %x[height], #0x1\n"  // height * 2
      "mov x27, #0x0\n"
      "mov x26, %x[in]\n"
      "lsr x9, x9, #0x1\n"  // n_loops = (n_passes - 1) / 2
      "and x25, x21, #0x1\n"  // odd_tail = bool(n_passes & 0x1)
      "csel x10, x10, x11, NE\n"
      "ldr x24, [x26, #0x0]\n"
      "ptrue p11.h\n"
      "whilelt p10.h, XZR, x20\n"
      "ldr x23, [x26, #0x8]\n"
      "mov x22, %x[row_offset]\n"
      "mov x21, %x[out]\n"
      "whilelt p9.h, x27, %x[width]\n"
      "whilelt p8.h, x27, %x[width]\n"
      "add x26, x26, #0x10\n"
      "mov x12, #0x0\n"
      "cbz x28, 2f\n"
      "1:"  // K loop: Charge: Loop
      ".inst 0x25286141  // psel p1.h, p8.h/Z, p10.h[w12]\n"
      ".inst 0x25386140  // psel p0.h, p8.h/Z, p10.h[w12, #1]\n"
      ".inst 0xe0560700  // ld1h { za0h.h[x12] }, p1/Z, [x24, x22, LSL #1]\n"
      "ldr x24, [x26, #0x0]\n"
      ".inst 0xe05602e1  // ld1h { za0h.h[x12, #1] }, p0/Z, [x23, x22, LSL #1]\n"
      "add x12, x12, #0x2\n"
      "ldr x23, [x26, #0x8]\n"
      "add x26, x26, #0x10\n"
      "cmp x12, x28\n"
      "blt 1b\n"
      "2:"  // K loop: Charge: End
      ".inst 0x25286141  // psel p1.h, p8.h/Z, p10.h[w12]\n"
      ".inst 0x25386140  // psel p0.h, p8.h/Z, p10.h[w12, #1]\n"
      "mov x26, %x[in]\n"
      "inch x27\n"
      ".inst 0xe0560700  // ld1h { za0h.h[x12] }, p1/Z, [x24, x22, LSL #1]\n"
      "ldr x24, [x26, #0x0]\n"
      ".inst 0xe05602e1  // ld1h { za0h.h[x12, #1] }, p0/Z, [x23, x22, LSL #1]\n"
      "ldr x23, [x26, #0x8]\n"
      "add x26, x26, #0x10\n"
      "inch x22\n"
      "cbz x9, 8f\n"
      "mov x20, x9\n"
      "3:"  // K loop: Main loop
      "whilelt p8.h, x27, %x[width]\n"
      "mov x13, #0x0\n"
      "cbz x28, 5f\n"
      "4:"  // K loop: Main loop: First: Loop
      ".inst 0x25296143  // psel p3.h, p8.h/Z, p10.h[w13]\n"
      ".inst 0x25396142  // psel p2.h, p8.h/Z, p10.h[w13, #1]\n"
      ".inst 0x25296d21  // psel p1.h, p11.h/Z, p9.h[w13]\n"
      ".inst 0x25396d20  // psel p0.h, p11.h/Z, p9.h[w13, #1]\n"
      ".inst 0xe0562f08  // ld1h { za1h.h[x13] }, p3/Z, [x24, x22, LSL #1]\n"
      "ldr x24, [x26, #0x0]\n"
      ".inst 0xe0562ae9  // ld1h { za1h.h[x13, #1] }, p2/Z, [x23, x22, LSL #1]\n"
      "ldr x23, [x26, #0x8]\n"
      ".inst 0xe07fa6a0  // st1h { za0v.h[x13] }, p1/Z, [x21, XZR, LSL #1]\n"
      "add x26, x26, #0x10\n"
      ".inst 0xe06ba2a1  // st1h { za0v.h[x13, #1] }, p0/Z, [x21, x11, LSL #1]\n"
      "add x13, x13, #0x2\n"
      "addvl x21, x21, #2\n"
      "cmp x13, x28\n"
      "blt 4b\n"
      "5:"  // K loop: Main loop: First: Tail
      ".inst 0x25296143  // psel p3.h, p8.h/Z, p10.h[w13]\n"
      ".inst 0x25396142  // psel p2.h, p8.h/Z, p10.h[w13, #1]\n"
      ".inst 0x25296d21  // psel p1.h, p11.h/Z, p9.h[w13]\n"
      ".inst 0x25396d20  // psel p0.h, p11.h/Z, p9.h[w13, #1]\n"
      "mov x26, %x[in]\n"
      "whilelt p9.h, x27, %x[width]\n"
      ".inst 0xe0562f08  // ld1h { za1h.h[x13] }, p3/Z, [x24, x22, LSL #1]\n"
      "ldr x24, [x26, #0x0]\n"
      "inch x27\n"
      "mov x12, #0x0\n"
      ".inst 0xe0562ae9  // ld1h { za1h.h[x13, #1] }, p2/Z, [x23, x22, LSL #1]\n"
      "ldr x23, [x26, #0x8]\n"
      ".inst 0xe07fa6a0  // st1h { za0v.h[x13] }, p1/Z, [x21, XZR, LSL #1]\n"
      "add x26, x26, #0x10\n"
      ".inst 0xe06ba2a1  // st1h { za0v.h[x13, #1] }, p0/Z, [x21, x11, LSL #1]\n"
      "addvl x21, x21, #2\n"
      "inch x22\n"
      "whilelt p8.h, x27, %x[width]\n"
      "cbz x28, 7f\n"
      "6:"  // K loop: Main loop: Second: Loop
      ".inst 0x25286143  // psel p3.h, p8.h/Z, p10.h[w12]\n"
      ".inst 0x25386142  // psel p2.h, p8.h/Z, p10.h[w12, #1]\n"
      ".inst 0x25286d21  // psel p1.h, p11.h/Z, p9.h[w12]\n"
      ".inst 0x25386d20  // psel p0.h, p11.h/Z, p9.h[w12, #1]\n"
      ".inst 0xe0560f00  // ld1h { za0h.h[x12] }, p3/Z, [x24, x22, LSL #1]\n"
      "ldr x24, [x26, #0x0]\n"
      ".inst 0xe0560ae1  // ld1h { za0h.h[x12, #1] }, p2/Z, [x23, x22, LSL #1]\n"
      "ldr x23, [x26, #0x8]\n"
      ".inst 0xe07f86a8  // st1h { za1v.h[x12] }, p1/Z, [x21, XZR, LSL #1]\n"
      "add x26, x26, #0x10\n"
      ".inst 0xe06b82a9  // st1h { za1v.h[x12, #1] }, p0/Z, [x21, x11, LSL #1]\n"
      "add x12, x12, #0x2\n"
      "addvl x21, x21, #2\n"
      "cmp x12, x28\n"
      "blt 6b\n"
      "7:"  // K loop: Main loop: Second: Tail
      ".inst 0x25286143  // psel p3.h, p8.h/Z, p10.h[w12]\n"
      ".inst 0x25386142  // psel p2.h, p8.h/Z, p10.h[w12, #1]\n"
      ".inst 0x25286d21  // psel p1.h, p11.h/Z, p9.h[w12]\n"
      ".inst 0x25386d20  // psel p0.h, p11.h/Z, p9.h[w12, #1]\n"
      "mov x26, %x[in]\n"
      "whilelt p9.h, x27, %x[width]\n"
      ".inst 0xe0560f00  // ld1h { za0h.h[x12] }, p3/Z, [x24, x22, LSL #1]\n"
      "ldr x24, [x26, #0x0]\n"
      "subs x20, x20, #0x1\n"
      "inch x27\n"
      ".inst 0xe0560ae1  // ld1h { za0h.h[x12, #1] }, p2/Z, [x23, x22, LSL #1]\n"
      "ldr x23, [x26, #0x8]\n"
      ".inst 0xe07f86a8  // st1h { za1v.h[x12] }, p1/Z, [x21, XZR, LSL #1]\n"
      "add x26, x26, #0x10\n"
      ".inst 0xe06b82a9  // st1h { za1v.h[x12, #1] }, p0/Z, [x21, x11, LSL #1]\n"
      "addvl x21, x21, #2\n"
      "inch x22\n"
      "bgt 3b\n"
      "8:"  // K loop: Tails
      "cbnz x25, 11f\n"
      "mov x26, %x[in]\n"
      "whilelt p8.h, x27, %x[width]\n"
      "mov x12, #0x0\n"
      "9:"  // K loop: Tails: Even: First
      ".inst 0x25286d21  // psel p1.h, p11.h/Z, p9.h[w12]\n"
      ".inst 0x25286140  // psel p0.h, p8.h/Z, p10.h[w12]\n"
      ".inst 0xe07f86a0  // st1h { za0v.h[x12] }, p1/Z, [x21, XZR, LSL #1]\n"
      "addvl x21, x21, #1\n"
      "ldr x20, [x26, #0x0]\n"
      "add x26, x26, #0x8\n"
      ".inst 0xe0560288  // ld1h { za1h.h[x12] }, p0/Z, [x20, x22, LSL #1]\n"
      "add x12, x12, #0x1\n"
      "cmp x12, x11\n"
      "blt 9b\n"
      "whilelt p9.h, x27, %x[width]\n"
      "whilelt p8.h, x27, %x[width]\n"
      "mov x12, #0x0\n"
      "10:"  // K loop: Tails: Even: Second
      ".inst 0x25286d20  // psel p0.h, p11.h/Z, p9.h[w12]\n"
      ".inst 0xe07f82a8  // st1h { za1v.h[x12] }, p0/Z, [x21, XZR, LSL #1]\n"
      "add x12, x12, #0x1\n"
      "addvl x21, x21, #1\n"
      "cmp x12, x10\n"
      "blt 10b\n"
      "whilelt p8.h, x27, %x[width]\n"
      "b 13f\n"
      "11:"  // K loop: Tails: Odd
      "mov x12, #0x0\n"
      "12:"  // K loop: Tails: Odd: Loop
      ".inst 0x25286d20  // psel p0.h, p11.h/Z, p9.h[w12]\n"
      ".inst 0xe07f82a0  // st1h { za0v.h[x12] }, p0/Z, [x21, XZR, LSL #1]\n"
      "add x12, x12, #0x1\n"
      "addvl x21, x21, #1\n"
      "cmp x12, x10\n"
      "blt 12b\n"
      "13:"  // K loop: End
      "mov %x[out], x21\n"
      ".inst 0xd503467f  // SMSTOP\n"
      : [out] "+&r" (out)
      : [height] "r" (height), [in] "r" (in), [row_offset] "r" (row_offset), [width] "r" (width)
      : "cc", "memory", "p0", "p1", "p2", "p3", "p4", "p5", "p6", "p7", "p8", "p9", "p10", "p11", "p12", "p13", "p14", "p15", "x9", "x10", "x11", "x12", "x13", "x20", "x21", "x22", "x23", "x24", "x25", "x26", "x27", "x28", "z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z8", "z9", "z10", "z11", "z12", "z13", "z14", "z15", "z16", "z17", "z18", "z19", "z20", "z21", "z22", "z23", "z24", "z25", "z26", "z27", "z28", "z29", "z30", "z31"
    );
}

#endif  // defined(ARM_COMPUTE_ENABLE_SME)
