import { Readable } from "node:stream";
import { crc32, createInflate } from "node:zlib";
import { HttpError } from "./http";

const invalid = () => new HttpError(400, "Invalid feedback screenshot.");

// The native capture writer emits noninterlaced 8-bit RGB or RGBA, with
// consecutive IDAT chunks between IHDR and IEND.
export async function feedbackImageSize(bytes: Uint8Array): Promise<[number, number]> {
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.length);
  if (bytes.length < 33 || view.getUint32(0) !== 0x89504e47 || view.getUint32(4) !== 0x0d0a1a0a ||
      view.getUint32(8) !== 13 || view.getUint32(12) !== 0x49484452 ||
      crc32(bytes.subarray(12, 29)) !== view.getUint32(29)) throw invalid();
  const width = view.getUint32(16), height = view.getUint32(20), color = bytes[25];
  if (!width || !height || width > 16384 || height > 16384 || width * height > 64000000 ||
      bytes[24] !== 8 || (color !== 2 && color !== 6) || bytes[26] || bytes[27] || bytes[28]) throw invalid();

  const parts: Uint8Array[] = [];
  let cursor = 33, compressedBytes = 0, ended = false;
  while (cursor <= bytes.length - 12) {
    const size = view.getUint32(cursor), kind = view.getUint32(cursor + 4);
    if (size > bytes.length - cursor - 12) throw invalid();
    const end = cursor + 8 + size;
    if (crc32(bytes.subarray(cursor + 4, end)) !== view.getUint32(end)) throw invalid();
    if (kind === 0x49454e44) {
      if (size || end + 4 !== bytes.length) throw invalid();
      ended = true;
      break;
    }
    if (kind !== 0x49444154 || parts.length >= 4096) throw invalid();
    parts.push(bytes.subarray(cursor + 8, end));
    compressedBytes += size;
    cursor = end + 4;
  }
  if (!ended || !compressedBytes) throw invalid();

  const rowBytes = width * (color === 2 ? 3 : 4) + 1;
  const expected = rowBytes * height;
  const input = Readable.from(parts, { objectMode: false });
  const inflate = createInflate();
  let produced = 0, nextRow = 0;
  try {
    for await (const chunk of input.pipe(inflate)) {
      const end = produced + chunk.length;
      if (end > expected) throw invalid();
      while (nextRow < end) {
        if (chunk[nextRow - produced] > 4) throw invalid();
        nextRow += rowBytes;
      }
      produced = end;
    }
    if (produced !== expected || inflate.bytesWritten !== compressedBytes) throw invalid();
  } catch {
    throw invalid();
  } finally {
    input.destroy();
    inflate.destroy();
  }
  return [width, height];
}
