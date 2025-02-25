////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Random.wgsl

// vec2u.x = high bits and vec2u.y = low bits
////////////////////////////////////////////////////////////////////////////////
fn Add64(a: vec2u, b: vec2u) -> vec2u {
	let rl: u32 = a.y + b.y;
	let rh: u32 = select(0u, 1u, rl < a.y);
	return vec2u(a.x + b.x + rh, rl);
};

////////////////////////////////////////////////////////////////////////////////
fn Mul64_32(a: u32, b: u32) -> vec2u {
	let ah: u32 = a >> 16u;
	let al: u32 = a & 0xffffu;
	let bh: u32 = b >> 16u;
	let bl: u32 = b & 0xffffu;
	let rl: u32 = al * bl;
	let rm1: u32 = ah * bl;
	let rm2: u32 = al * bh;
	let rh: u32 = ah * bh;

	let rm1h: u32 = rm1 >> 16u;
	let rm1l: u32 = (rm1 & 0xffffu) << 16u;
	let rm2h: u32 = rm2 >> 16u;
	let rm2l: u32 = (rm2 & 0xffffu) << 16u;
	var high: u32 = rh + rm1h + rm2h;
	var low: u32 = rl + rm1l;
	high += select(0u, 1u, low < rl);
	low += rm2l;
	high += select(0u, 1u, low < rm2l);
	return vec2u(high, low);
};

////////////////////////////////////////////////////////////////////////////////
fn Mul64(a: vec2u, b: vec2u) -> vec2u {
	let a0b0: vec2u = Mul64_32(a.y, b.y);
	let a0b1: u32 = a.y * b.x;
	let a1b0: u32 = a.x * b.y;
	return vec2u(a0b1 + a1b0 + a0b0.x, a0b0.y);
};

////////////////////////////////////////////////////////////////////////////////
fn Xor64(a: vec2u, b: vec2u) -> vec2u {
	return vec2u(a.x ^ b.x, a.y ^ b.y);
};

////////////////////////////////////////////////////////////////////////////////
fn LeftShift64(a: vec2u, num: u32) -> vec2u {
	if (num >= 32u) {
		let new_low: u32 = a.x >> (num - 32u);
		return vec2u(0, new_low);
	}
	let new_low: u32 = (a.y >> num) | (a.x << (32u - num));
	let new_high: u32 = (a.x >> num);
	return vec2u(new_high, new_low);
};

const g_32Inc: vec2u = vec2u(335903614u, 4150755663u);
const g_32Mult: vec2u = vec2u(1481765933u, 1284865837u);
////////////////////////////////////////////////////////////////////////////////
fn AdvanceSeedState(seed: vec2u) -> vec2u {
	return Add64(Mul64(seed, g_32Mult), g_32Inc);
}

////////////////////////////////////////////////////////////////////////////////
fn GetHash(seed: vec2u) -> u32 {
	let xorshifted: u32 = LeftShift64(Xor64(LeftShift64(seed, 18u), seed), 27u).y;
	let rot: u32 = LeftShift64(seed, 59u).y;
	return (xorshifted >> rot) | (xorshifted << (((rot ^ 0xFFFFFFFFu) + 1u) & 31u));
}

////////////////////////////////////////////////////////////////////////////////
fn GetPCGRandom(seed: ptr<function, vec2u>) -> u32 {
	let value: u32 = GetHash(*seed);
	(*seed) = AdvanceSeedState(*seed);
	return value;
}

////////////////////////////////////////////////////////////////////////////////
fn NextRandom(min_val: f32, max_val: f32, seed: ptr<function, vec2u>) -> f32 {
	let zero_to_one: f32 = f32(GetPCGRandom(seed)) * 0.0000000002328306437;
    return zero_to_one * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
fn GetRandomFromSeed(min_val: f32, max_val: f32, seed: u32) -> f32 {
	let value: u32 = GetHash(AdvanceSeedState(vec2u(0x4d595df4u, seed)));
	let zero_to_one: f32 = f32(value) * 0.0000000002328306437;
    return zero_to_one * (max_val - min_val) + min_val;
}

////////////////////////////////////////////////////////////////////////////////
fn InitHash() -> vec2u {
	return vec2u(0x4d595df4u, 0xef053da3u);
}

////////////////////////////////////////////////////////////////////////////////
fn AddHash(hash: vec2u, item: u32) -> vec2u {
	return AdvanceSeedState(vec2u(hash.x ^ item, hash.y ^ item));
}

////////////////////////////////////////////////////////////////////////////////
fn AddHashFloat(hash: vec2u, item: f32) -> vec2u {
	let int_val: u32 = bitcast<u32>(item);
	return AdvanceSeedState(vec2u(hash.x ^ int_val, hash.y ^ int_val));
}