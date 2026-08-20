def decode_s32(words: list) -> int:
    """Decode an array of two 16-bit registers into a signed 32-bit integer."""
    if not words or len(words) < 2: return None
    val = (words[0] << 16) | words[1]
    if val >= (1 << 31): val -= (1 << 32)
    # SMA often returns 0x80000000 for NaN/Invalid
    if val == -2147483648: return None
    return val

def decode_u64(words: list) -> int:
    """Decode an array of four 16-bit registers into an unsigned 64-bit integer."""
    if not words or len(words) < 4: return None
    val = (words[0] << 48) | (words[1] << 32) | (words[2] << 16) | words[3]
    return val
