#include "return_codes.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MIN_EXP16 -15
#define MIN_EXP32 -127
#define MAX_EXP16 16
#define MAX_EXP32 128
#define MIN_MANT32 (1 << 23)
#define MIN_MANT16 (1 << 10)
#define MAX_MANT32 0xFFFFFF
#define MAX_MANT16 0x7FF
#define EXP_MASK32 0x7F800000
#define EXP_MASK16 0x7C00
#define MANT_MASK32 0x7FFFFF
#define MANT_MASK16 0x3FF
#define PRINT_MANT_MASK32 0xFFFFFF
#define PRINT_MANT_MASK16 0xFFF
#define MODE_IF(mode, a, b) (((mode) == 'f') ? (a) : (b))
#define IS_ZERO(a)                                                                                                     \
	MODE_IF((a)->mode, ((a)->mant == MIN_MANT32 && (a)->exp == MIN_EXP32), ((a)->mant == MIN_MANT16 && (a)->exp == MIN_EXP16))

#define IS_NAN(a)                                                                                                      \
	MODE_IF((a)->mode, ((a)->exp == MAX_EXP32 && (a)->mant != MIN_MANT32), ((a)->exp == MAX_EXP16 && (a)->mant != MIN_MANT16))

#define IS_INF(a)                                                                                                      \
	MODE_IF((a)->mode, ((a)->exp == MAX_EXP32 && (a)->mant == MIN_MANT32), ((a)->exp == MAX_EXP16 && (a)->mant == MIN_MANT16))

#define IS_NEG_INF(a) (IS_INF(a) && (a)->sign)

#define IS_POS_INF(a) (IS_INF(a) && !(a)->sign)
#define DENORMAL(a)                                                                                                    \
	MODE_IF((a)->mode, ((a)->exp == MIN_EXP32 && !IS_ZERO(a) && !(a)->isNorm), ((a)->exp == MIN_EXP16 && !IS_ZERO(a) && !(a)->isNorm))

#define IS_DENORMAL(a, minExp) (((a).exp == (minExp)) && (!IS_ZERO(&(a))))

typedef struct Float
{
	int64_t mant;
	int32_t exp;
	int8_t sign;
	bool isNorm;
	char mode;
} Float;

Float parseIntToFloat(int32_t a, char mode)
{
	Float res;
	res.mode = mode;
	int32_t mant = MODE_IF(mode, MANT_MASK32, MANT_MASK16);
	int32_t exp = MODE_IF(mode, EXP_MASK32, EXP_MASK16);
	int32_t shift = MODE_IF(mode, 31, 15);
	res.sign = ((a >> shift) & 1);
	int32_t minMant = MODE_IF(mode, MIN_MANT32, MIN_MANT16);
	int32_t minExp = MODE_IF(mode, MIN_EXP32, MIN_EXP16);
	res.mant = (a & mant) + minMant;
	int32_t mantLen = MODE_IF(mode, 23, 10);
	res.exp = ((a & exp) >> mantLen) + minExp;
	res.isNorm = true;
	res.mant -= IS_DENORMAL(res, minExp) * minMant;
	res.isNorm = !IS_DENORMAL(res, minExp);
	return res;
}

Float Inf(int8_t sign, char mode)
{
	Float result;
	result.mode = mode;
	result.exp = MODE_IF(mode, MAX_EXP32, MAX_EXP16);
	result.sign = sign;
	result.mant = MODE_IF(mode, MIN_MANT32, MIN_MANT16);
	result.isNorm = true;
	return result;
}

Float Nan(char mode)
{
	Float result;
	result.mode = mode;
	result.exp = MODE_IF(mode, MAX_EXP32, MAX_EXP16);
	result.sign = 0;
	result.mant = MODE_IF(mode, MIN_MANT32, MIN_MANT16) + 1;
	result.isNorm = true;
	return result;
}

Float zero(int8_t sign, char mode)
{
	Float result;
	result.mode = mode;
	result.exp = MODE_IF(mode, MIN_EXP32, MIN_EXP16);
	result.sign = sign;
	result.mant = MODE_IF(mode, MIN_MANT32, MIN_MANT16);
	result.isNorm = true;
	return result;
}

void addRightZeros(Float *p, int32_t minMant)
{
	Float copy = *p;
	while (copy.mant < minMant && copy.mant > 0)
	{
		copy.mant <<= 1;
		copy.exp--;
	}
	copy.isNorm = true;
	*p = copy;
}

void normalize(Float *p, int32_t minMant)
{
	if (p->mant < minMant)
	{
		addRightZeros(p, minMant);
		p->exp += 1;
	}
}

void printUsual(Float *a)
{
	Float copyA = *a;
	copyA.mant = MODE_IF(copyA.mode, (copyA.mant << 1) & PRINT_MANT_MASK32, (copyA.mant << 2) & PRINT_MANT_MASK16);
	if (copyA.sign)
	{
		printf("-");
	}
	printf("0x1.%0*" PRIx64 "p%+d\n", MODE_IF(copyA.mode, 6, 3), copyA.mant, copyA.exp);
}

void print(Float *a)
{
	if (IS_NAN(a))
	{
		printf("nan\n");
	}
	else if (IS_NEG_INF(a))
	{
		printf("-inf\n");
	}
	else if (IS_POS_INF(a))
	{
		printf("inf\n");
	}
	else if (IS_ZERO(a))
	{
		if (a->sign)
		{
			printf("-");
		}
		printf(MODE_IF(a->mode, "0x0.000000p+0\n", "0x0.000p+0\n"));
	}
	else
	{
		if (DENORMAL(a))
		{
			normalize(a, MODE_IF(a->mode, MIN_MANT32, MIN_MANT16));
		}
		printUsual(a);
	}
}

int64_t getMaskForDiff(int32_t diff)
{
	return (1LL << diff) - 1;
}

Float minNumber(int8_t sign, char mode)
{
	Float result;
	result.mode = mode;
	result.exp = MODE_IF(mode, MIN_EXP32, MIN_EXP16);
	result.sign = sign;
	result.mant = 1;
	result.isNorm = false;
	return result;
}

Float maxNumber(int8_t sign, char mode)
{
	Float result;
	result.mode = mode;
	result.exp = MODE_IF(mode, MAX_EXP32 - 1, MAX_EXP16 - 1);
	result.sign = sign;
	result.mant = getMaskForDiff(MODE_IF(mode, 24, 11));
	result.isNorm = false;
	return result;
}

int8_t towardNearestRoundTail(Float *p, int64_t remainder)
{
	if (remainder > 0 || (p->mant & 1))
	{
		p->mant++;
		return 1;
	}
	return 0;
}

Float checkRes(Float *result, int8_t rounding)
{
	Float copyResult = *result;
	if (copyResult.exp >= MODE_IF(copyResult.mode, MAX_EXP32, MAX_EXP16))
	{
		if (rounding == 0 || rounding == 2 && copyResult.sign || rounding == 3 && !copyResult.sign)
		{
			return maxNumber(copyResult.sign, copyResult.mode);
		}
		return Inf(copyResult.sign, copyResult.mode);
	}
	if (copyResult.exp < MODE_IF(copyResult.mode, -149, -24))
	{
		if (rounding == 3 && copyResult.sign || rounding == 2 && !copyResult.sign ||
			rounding == 1 && copyResult.exp == MODE_IF(copyResult.mode, -150, -25) &&
				copyResult.mant > MODE_IF(copyResult.mode, MIN_MANT32, MIN_MANT16))
		{
			return minNumber(copyResult.sign, copyResult.mode);
		}
		return zero(copyResult.sign, copyResult.mode);
	}
	while (!copyResult.isNorm && copyResult.mant >= 1 << MODE_IF(copyResult.mode, 24, 11))
	{
		copyResult.mant >>= 1;
		copyResult.exp++;
	}
	return copyResult;
}

void secondRounding(Float *result, int8_t rounding, int32_t firstRemainder, int8_t wasAdded)
{
	if (result->exp > MODE_IF(result->mode, MIN_EXP32, MIN_EXP16))
	{
		return;
	}
	Float copyResult = *result;
	int16_t shift = MODE_IF(copyResult.mode, 24, 11) - (MODE_IF(copyResult.mode, 150, 25) + copyResult.exp);
	if (shift <= 0 || copyResult.exp < MODE_IF(copyResult.mode, -149, -24))
	{
		return;
	}
	copyResult.mant -= wasAdded;
	int32_t remainder = copyResult.mant & getMaskForDiff(shift);
	copyResult.mant >>= shift;
	copyResult.mant +=
		((rounding == 2 && !copyResult.sign || rounding == 3 && copyResult.sign) && (remainder > 0 || firstRemainder > 0));
	if (rounding == 1 && remainder >= (1 << (shift - 1)))
	{
		towardNearestRoundTail(&copyResult, (remainder & getMaskForDiff(shift - 1)) + firstRemainder);
	}
	copyResult.mant <<= shift;
	while (copyResult.mant >= 1 << MODE_IF(copyResult.mode, 24, 11))
	{
		copyResult.exp++;
		copyResult.mant >>= 1;
	}
	if (IS_ZERO(&copyResult))
	{
		while (copyResult.exp < MODE_IF(copyResult.mode, -126, -14))
		{
			copyResult.mant >>= 1;
			copyResult.exp++;
		}
		copyResult.exp--;
		copyResult.isNorm = false;
	}
	*result = copyResult;
}
void towardZeroRounding(Float *p, int16_t mantLen, int32_t maxMant, int8_t rounding)
{
	int32_t firstRemainder = 0;
	int8_t wasAdded = 0;
	Float copyP = *p;
	if (copyP.mant >= (1LL << (mantLen << 1)))
	{
		copyP.mant >>= mantLen;
	}
	while (copyP.mant > maxMant)
	{
		copyP.mant >>= 1;
		copyP.exp++;
	}
	*p = copyP;
	secondRounding(p, rounding, firstRemainder, wasAdded);
}

int16_t getMaxLen(int64_t mant, int16_t start)
{
	int16_t maxLen = 0;
	for (int16_t i = start; i <= 60; i++)
	{
		if (mant >= (1LL << i))
		{
			maxLen = i;
		}
	}
	return maxLen;
}

int32_t increaseExp(int32_t exp, int16_t start, int16_t maxLen)
{
	exp += (maxLen > ((start << 1) - 2) ? (maxLen - ((start << 1) - 2)) : 0);
	exp += (maxLen < ((start << 1) - 3) && maxLen >= start ? (maxLen - start + 1) : 0);
	return exp;
}

void towardNearestRounding(Float *p, int16_t start, int8_t rounding)
{
	int16_t maxLen = getMaxLen(p->mant, start);
	int32_t remainder, firstRemainder = p->mant & getMaskForDiff(maxLen - start + 1);
	int8_t wasAdded = 0;
	if (maxLen > 0)
	{
		if (!((p->mant >> (maxLen - start)) & 1))
		{
			p->mant >>= (maxLen - start + 1);
		}
		else
		{
			remainder = p->mant & getMaskForDiff(maxLen - start);
			p->mant >>= (maxLen - start + 1);
			wasAdded = towardNearestRoundTail(p, remainder);
		}
		p->exp = increaseExp(p->exp, start, maxLen);
	}
	secondRounding(p, rounding, firstRemainder, wasAdded);
}

void findMaxLen(Float *p, int16_t start, int8_t rounding)
{
	int16_t maxLen = getMaxLen(p->mant, start);
	int32_t firstRemainder = p->mant & getMaskForDiff(maxLen - start + 1);
	int8_t wasAdded = 0;
	if (maxLen > 0)
	{
		wasAdded = ((p->mant & getMaskForDiff(maxLen - start + 1)) > 0);
		p->mant = (p->mant >> (maxLen - start + 1)) + wasAdded;
		p->exp = increaseExp(p->exp, start, maxLen);
	}
	secondRounding(p, rounding, firstRemainder, wasAdded);
}

void towardInfRounding(Float *p, int8_t sign, int16_t start, int16_t mantLen, int32_t maxMant, int32_t minMant, int8_t rounding)
{
	if (p->sign == sign)
	{
		findMaxLen(p, start, rounding);
	}
	else
	{
		towardZeroRounding(p, mantLen, maxMant, rounding);
	}
}

void roundFloat(Float *p, int8_t rounding, int32_t maxMant, int16_t start, int16_t mantLen, int32_t minMant)
{
	switch (rounding)
	{
	case 0:
		towardZeroRounding(p, mantLen, maxMant, rounding);
		break;
	case 1:
		towardNearestRounding(p, start, rounding);
		break;
	case 2:
		towardInfRounding(p, 0, start, mantLen, maxMant, minMant, rounding);
		break;
	case 3:
		towardInfRounding(p, 1, start, mantLen, maxMant, minMant, rounding);
		break;
	}
	if (p->isNorm)
	{
		addRightZeros(p, minMant);
	}
}
void checkNorm(Float *a, Float *b)
{
	if (DENORMAL(a))
	{
		normalize(a, MODE_IF(a->mode, MIN_MANT32, MIN_MANT16));
	}
	if (DENORMAL(b))
	{
		normalize(b, MODE_IF(b->mode, MIN_MANT32, MIN_MANT16));
	}
}

int32_t minimum(int32_t a, int32_t b)
{
	return a < b ? a : b;
}

void checkRoundingAdd(Float *result, int32_t x, int32_t diff, int8_t rounding, int32_t remainder, int16_t mantLen)
{
	if (diff > mantLen && rounding == 1 && remainder >= (1 << minimum(diff - mantLen, mantLen - 1)))
	{
		result->mant += x * ((remainder > (1 << minimum(diff - mantLen, mantLen - 1))) || (result->mant & 1));
	}
}

int32_t toOneExponent(Float *unshiftFl, Float *shiftFl, int32_t diff, int16_t mantLen, int32_t *remainder)
{
	diff = unshiftFl->exp - shiftFl->exp;
	shiftFl->exp += diff;
	*remainder = diff <= mantLen ? 0 : (shiftFl->mant & getMaskForDiff(minimum(mantLen, diff - mantLen)));
	shiftFl->mant <<= mantLen;
	unshiftFl->mant <<= mantLen;
	shiftFl->mant >>= minimum(diff, mantLen << 1);
	return diff;
}

void addZerosToLength(Float *result, int64_t len)
{
	Float copy = *result;
	while (copy.mant > 0 && copy.mant < len)
	{
		copy.mant <<= 1;
		copy.exp--;
	}
	*result = copy;
}

void abstractAdd(Float *a, Float *b, Float *result, int16_t mantLen, int8_t rounding)
{
	Float copyA = *a, copyB = *b, copyResult = *result;
	int32_t diff = 0, remainder = 0;
	if (copyA.exp > copyB.exp)
	{
		diff = toOneExponent(&copyA, &copyB, copyA.exp - copyB.exp, mantLen, &remainder);
	}
	else if (copyA.exp < copyB.exp)
	{
		diff = toOneExponent(&copyB, &copyA, copyB.exp - copyA.exp, mantLen, &remainder);
	}
	else
	{
		copyA.mant <<= mantLen;
		copyB.mant <<= mantLen;
	}
	copyResult.exp = copyA.exp;
	if (copyA.sign == copyB.sign)
	{
		copyResult.sign = copyA.sign;
		copyResult.mant = copyA.mant + copyB.mant;
		copyResult.mant += ((rounding == 2 && !copyResult.sign || rounding == 3 && copyResult.sign) && remainder > 0);
		checkRoundingAdd(&copyResult, 1, diff, rounding, remainder, mantLen);
	}
	else
	{
		if (copyA.mant > copyB.mant)
		{
			copyResult.sign = copyA.sign;
			copyResult.mant = copyA.mant - copyB.mant;
		}
		else
		{
			copyResult.sign = copyB.sign;
			copyResult.mant = copyB.mant - copyA.mant;
		}
		copyResult.mant -=
			(remainder > 0 && (!copyResult.sign && rounding == 3 || copyResult.sign && rounding == 2 || rounding == 0));
		checkRoundingAdd(&copyResult, -1, diff, rounding, remainder, mantLen);
	}
	addZerosToLength(&copyResult, 1LL << ((mantLen - 1) << 1));
	copyResult.exp--;
	*result = copyResult;
}

Float specialAdd(int8_t rounding, Float *a, Float *b, int16_t mantLen)
{
	if (IS_NAN(a) || IS_NAN(b) || IS_INF(a) && a->sign != b->sign && IS_INF(b))
	{
		return Nan(a->mode);
	}
	if ((a->mant == b->mant && a->exp == b->exp && a->sign != b->sign) || (IS_ZERO(a) && IS_ZERO(b) && a->sign != b->sign))
	{
		return zero(rounding == 3, a->mode);
	}
	if (IS_ZERO(a))
	{
		return *b;
	}
	if (IS_ZERO(b))
	{
		return *a;
	}
	if (IS_NEG_INF(a) || IS_NEG_INF(b))
	{
		return Inf(1, a->mode);
	}
	if (IS_POS_INF(a) || IS_POS_INF(b))
	{
		return Inf(0, a->mode);
	}
	Float result;
	result.mode = a->mode;
	checkNorm(a, b);
	abstractAdd(a, b, &result, mantLen, rounding);
	roundFloat(&result, rounding, MODE_IF(a->mode, MAX_MANT32, MAX_MANT16), mantLen, mantLen - 1, MODE_IF(a->mode, MIN_MANT32, MIN_MANT16));
	return checkRes(&result, rounding);
}

Float add(Float *a, Float *b, int8_t rounding)
{
	return specialAdd(rounding, a, b, MODE_IF(a->mode, 24, 11));
}

Float subtract(Float *a, Float *b, int8_t rounding)
{
	b->sign = b->sign != 1;
	return add(a, b, rounding);
}

void multiplyAbstract(Float *a, Float *b, Float *result)
{
	result->sign = ((a->sign + b->sign) & 1);
	result->mant = a->mant * b->mant;
	result->exp = a->exp + b->exp;
}

Float specialMultipy(int8_t rounding, Float *a, Float *b, int16_t mantLen)
{
	if (IS_NAN(a) || IS_NAN(b) || IS_ZERO(a) && IS_INF(b) || IS_ZERO(b) && IS_INF(a))
	{
		return Nan(a->mode);
	}
	if (IS_ZERO(a) || IS_ZERO(b))
	{
		return zero((a->sign + b->sign) & 1, a->mode);
	}
	if (IS_INF(a) || IS_INF(b))
	{
		return Inf((b->sign + a->sign) & 1, a->mode);
	}
	Float result;
	result.mode = a->mode;
	checkNorm(a, b);
	multiplyAbstract(a, b, &result);
	roundFloat(&result, rounding, MODE_IF(a->mode, MAX_MANT32, MAX_MANT16), mantLen, mantLen - 1, MODE_IF(a->mode, MIN_MANT32, MIN_MANT16));
	return checkRes(&result, rounding);
}

Float multiply(Float *a, Float *b, int8_t rounding)
{
	return specialMultipy(rounding, a, b, MODE_IF(a->mode, 24, 11));
}

void divideAbstract(Float *a, Float *b, Float *result, int16_t mantLen, int8_t rounding)
{
	Float copyA = *a, copyB = *b, copyResult = *result;
	copyResult.sign = (copyA.sign + copyB.sign) & 1;
	copyResult.exp = copyA.exp - copyB.exp;
	copyResult.mant = copyA.mant / copyB.mant;
	int32_t remainder = copyA.mant - copyB.mant * copyResult.mant;
	for (int32_t i = 1; i <= (mantLen << 1); i++)
	{
		copyResult.mant <<= 1;
		remainder <<= 1;
		copyResult.mant += (remainder >= copyB.mant);
		remainder -= copyB.mant * (remainder >= copyB.mant);
	}
	copyResult.mant += (remainder > 0 && (rounding == 2 && !copyResult.sign || rounding == 3 && copyResult.sign));
	*result = copyResult;
	addZerosToLength(result, 1LL << (mantLen << 1));
}

Float specialDivide(int8_t rounding, Float *a, Float *b, int16_t mantLen)
{
	if (IS_NAN(a) || IS_NAN(b) || IS_INF(a) && IS_INF(b) || IS_ZERO(a) && IS_ZERO(b))
	{
		return Nan(a->mode);
	}
	if (IS_INF(b) || IS_ZERO(a))
	{
		return zero((a->sign + b->sign) & 1, a->mode);
	}
	if (IS_INF(a) || IS_ZERO(b))
	{
		return Inf((a->sign + b->sign) & 1, a->mode);
	}
	checkNorm(a, b);
	Float result;
	result.mode = a->mode;
	divideAbstract(a, b, &result, mantLen - 1, rounding);
	roundFloat(&result, rounding, MODE_IF(a->mode, MAX_MANT32, MAX_MANT16), mantLen, mantLen - 1, MODE_IF(a->mode, MIN_MANT32, MIN_MANT16));
	return checkRes(&result, rounding);
}

Float divide(Float *a, Float *b, int8_t rounding)
{
	return specialDivide(rounding, a, b, MODE_IF(a->mode, 24, 11));
}

int main(int argc, char *argv[])
{
	if (argc != 4 && argc != 6)
	{
		fprintf(stderr, "%d arguments instead of 3 or 5\n", argc - 1);
		return ERROR_ARGUMENTS_INVALID;
	}
	if (strlen(argv[1]) > 1)
	{
		fprintf(stderr, "%s is incorrect format for mode\n", argv[1]);
		return ERROR_ARGUMENTS_INVALID;
	}
	char mode = argv[1][0];
	if (mode != 'f' && mode != 'h')
	{
		fprintf(stderr, "incorrect format: %c\n", mode);
		return ERROR_ARGUMENTS_INVALID;
	}
	int8_t rounding;
	int32_t left;
	sscanf(argv[2], "%hhd", &rounding);
	sscanf(argv[3], "%x", &left);
	if (rounding < 0 || rounding > 3)
	{
		fprintf(stderr, "incorrect rounding: %hhd\n", rounding);
		return ERROR_ARGUMENTS_INVALID;
	}
	Float result;
	if (argc == 6)
	{
		if (strlen(argv[4]) > 1)
		{
			fprintf(stderr, "%s is incorrect format for operation\n", argv[4]);
			return ERROR_ARGUMENTS_INVALID;
		}
		char op = argv[4][0];
		int32_t right;
		sscanf(argv[5], "%x", &right);
		Float a, b;
		a = parseIntToFloat(left, mode);
		b = parseIntToFloat(right, mode);
		switch (op)
		{
		case '+':
			result = add(&a, &b, rounding);
			break;
		case '-':
			result = subtract(&a, &b, rounding);
			break;
		case '*':
			result = multiply(&a, &b, rounding);
			break;
		case '/':
			result = divide(&a, &b, rounding);
			break;
		default:
			fprintf(stderr, "incorrect operation: %c\n", op);
			return ERROR_ARGUMENTS_INVALID;
		}
	}
	else
	{
		result = parseIntToFloat(left, mode);
	}
	print(&result);
	return SUCCESS;
}
