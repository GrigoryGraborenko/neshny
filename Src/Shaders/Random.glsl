////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Random.glsl

// uvec2.x = high bits and uvec2.y = low bits
////////////////////////////////////////////////////////////////////////////////
uvec2 Add64(uvec2 a, uvec2 b) {
	uint rl = a.y + b.y;
	uint rh = rl < a.y ? 1 : 0;
	return uvec2(a.x + b.x + rh, rl);
};

////////////////////////////////////////////////////////////////////////////////
uvec2 Mul64_32(uint a, uint b) {
	uint ah = a >> 16, al = a & 0xFFFFU;
	uint bh = b >> 16, bl = b & 0xFFFFU;
	uint rl = al * bl;
	uint rm1 = ah * bl;
	uint rm2 = al * bh;
	uint rh = ah * bh;

	uint rm1h = rm1 >> 16, rm1l = (rm1 & 0xFFFFU) << 16;
	uint rm2h = rm2 >> 16, rm2l = (rm2 & 0xFFFFU) << 16;
	uint high = rh + rm1h + rm2h;
	uint low = rl + rm1l;
	high += low < rl ? 1 : 0;
	low += rm2l;
	high += low < rm2l ? 1 : 0;
	return uvec2(high, low);
};

////////////////////////////////////////////////////////////////////////////////
uvec2 Mul64(uvec2 a, uvec2 b) {
	uvec2 a0b0 = Mul64_32(a.y, b.y);
	uint a0b1 = a.y * b.x;
	uint a1b0 = a.x * b.y;
	return uvec2(a0b1 + a1b0 + a0b0.x, a0b0.y);
};

////////////////////////////////////////////////////////////////////////////////
uvec2 Xor64(uvec2 a, uvec2 b) {
	return uvec2(a.x ^ b.x, a.y ^ b.y);
};

////////////////////////////////////////////////////////////////////////////////
uvec2 LeftShift64(uvec2 a, int num) {
	if (num >= 32) {
		uint new_low = a.x >> (num - 32);
		return uvec2(0, new_low);
	}
	uint new_low = (a.y >> num) | (a.x << (32 - num));
	uint new_high = (a.x >> num);
	return uvec2(new_high, new_low);
};

const uvec2 g_32Inc = uvec2(335903614, 4150755663);
const uvec2 g_32Mult = uvec2(1481765933, 1284865837);
////////////////////////////////////////////////////////////////////////////////
uvec2 AdvanceSeedState(uvec2 seed) {
	return Add64(Mul64(seed, g_32Mult), g_32Inc);
}

////////////////////////////////////////////////////////////////////////////////
uint GetHash(uvec2 seed) {
	uint xorshifted = LeftShift64(Xor64(LeftShift64(seed, 18), seed), 27).y;
	uint rot = LeftShift64(seed, 59).y;
	return (xorshifted >> rot) | (xorshifted << (((rot ^ 0xFFFFFFFF) + 1) & 31));
}

////////////////////////////////////////////////////////////////////////////////
uint GetPCGRandom(inout uvec2 seed) {
	uint value = GetHash(seed);
	seed = AdvanceSeedState(seed);
	return value;
}

////////////////////////////////////////////////////////////////////////////////
float GetNextRandom(float min_val, float max_val, inout uvec2 seed) {
	uint value = GetPCGRandom(seed);
	float zero_to_one = float(value) * 0.0000000002328306437;
	return zero_to_one * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
float GetRandom(float min_val, float max_val, uint seed) {
	uint value = GetHash(AdvanceSeedState(uvec2(0x4d595df4, seed)));
	float zero_to_one = float(value) * 0.0000000002328306437;
    return zero_to_one * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
uvec2 InitHash() {
	return uvec2(0x4d595df4, 0xef053da3);
}

////////////////////////////////////////////////////////////////////////////////
uvec2 AddHash(uvec2 hash, uint item) {
	return AdvanceSeedState(uvec2(hash.x ^ item, hash.y ^ item));
}

////////////////////////////////////////////////////////////////////////////////
uvec2 AddHash(uvec2 hash, float item) {
	uint int_val = floatBitsToInt(item);
	return AdvanceSeedState(uvec2(hash.x ^ int_val, hash.y ^ int_val));
}

////////////////////////////////////////////////////////////////////////////////
float TrigHashSingle(float num) {
	return fract(sin(num * 0.01 + 0.45) + cos(num * 1.04573 + 0.1) + sin(num * 11.32523 + 1.674) + sin(num * 1076.043 + 563.50));
}

////////////////////////////////////////////////////////////////////////////////
float TrigHash(float num) {
	return TrigHashSingle(TrigHashSingle(TrigHashSingle(TrigHashSingle(num))));
}

////////////////////////////////////////////////////////////////////////////////
// TODO: profile if this are faster than the proper RNG and good enough for use
float GetTrigRandom(float min_val, float max_val, float seed) {
    return TrigHash(seed) * (max_val - min_val) + min_val;
}