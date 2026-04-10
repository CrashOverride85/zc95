from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


UF2_BLOCK_SIZE = 512
UF2_MAGIC_START0 = 0x0A324655
UF2_MAGIC_START1 = 0x9E5D5157
UF2_MAGIC_END = 0x0AB16F30
UF2_DATA_OFFSET = 32
UF2_MAX_PAYLOAD_SIZE = 476
BOOTLOADER_SKIP_BYTES = 48 * 1024
IMAGE_HEADER_SIZE = 128
FIRMWARE_VERSION_LENGTH = 32


class Uf2Error(ValueError):
    """Raised when a UF2 file is invalid."""


@dataclass(frozen=True)
class FirmwareInfo:
    magic: int
    firmware_version: str


@dataclass(frozen=True)
class BinImage:
    name: str
    path: Path
    data: bytes
    base_address: int
    firmware_info: FirmwareInfo

    @property
    def upload_data(self) -> bytes:
        return self.data[BOOTLOADER_SKIP_BYTES:]


def _u32le(data: bytes, offset: int) -> int:
    return int.from_bytes(data[offset : offset + 4], "little")


def uf2_to_bin_bytes(raw: bytes) -> tuple[bytes, int]:
    if len(raw) == 0 or len(raw) % UF2_BLOCK_SIZE != 0:
        raise Uf2Error("UF2 file size must be a non-zero multiple of 512 bytes.")

    blocks = {}
    block_count = len(raw) // UF2_BLOCK_SIZE
    expected_total_blocks: int | None = None
    seen_block_numbers: set[int] = set()

    for index in range(block_count):
        block = raw[index * UF2_BLOCK_SIZE : (index + 1) * UF2_BLOCK_SIZE]
        if _u32le(block, 0) != UF2_MAGIC_START0 or _u32le(block, 4) != UF2_MAGIC_START1:
            raise Uf2Error(f"UF2 magic mismatch in block {index}.")
        if _u32le(block, 508) != UF2_MAGIC_END:
            raise Uf2Error(f"UF2 end magic mismatch in block {index}.")

        payload_size = _u32le(block, 16)
        if payload_size <= 0 or payload_size > UF2_MAX_PAYLOAD_SIZE:
            raise Uf2Error(f"Invalid UF2 payload size {payload_size} in block {index}.")

        block_no = _u32le(block, 20)
        total_blocks = _u32le(block, 24)
        target_addr = _u32le(block, 12)

        if expected_total_blocks is None:
            if total_blocks <= 0:
                raise Uf2Error("UF2 block count must be greater than zero.")
            expected_total_blocks = total_blocks
        elif total_blocks != expected_total_blocks:
            raise Uf2Error("UF2 blocks disagree on total block count.")

        payload = block[UF2_DATA_OFFSET : UF2_DATA_OFFSET + payload_size]
        if len(payload) != payload_size:
            raise Uf2Error(f"Truncated UF2 payload in block {index}.")

        existing = blocks.get(target_addr)
        if existing is not None and existing != payload:
            raise Uf2Error(f"Conflicting UF2 payload at address 0x{target_addr:08X}.")
        blocks[target_addr] = payload

        if block_no >= total_blocks:
            raise Uf2Error(f"Invalid block number {block_no} for block {index}.")
        seen_block_numbers.add(block_no)

    if not blocks:
        raise Uf2Error("UF2 file did not contain any payload blocks.")
    if expected_total_blocks is not None and len(seen_block_numbers) != expected_total_blocks:
        raise Uf2Error("UF2 file is missing one or more blocks.")

    sorted_addresses = sorted(blocks)
    base_address = sorted_addresses[0]
    end_address = max(address + len(blocks[address]) for address in sorted_addresses)
    image = bytearray([0xFF] * (end_address - base_address))

    for address in sorted_addresses:
        offset = address - base_address
        payload = blocks[address]
        image[offset : offset + len(payload)] = payload

    return bytes(image), base_address


def extract_firmware_info(image_data: bytes) -> FirmwareInfo:
    if len(image_data) < BOOTLOADER_SKIP_BYTES + IMAGE_HEADER_SIZE:
        raise Uf2Error(
            "Converted image is too small to contain the expected bootloader and image header."
        )

    header = image_data[BOOTLOADER_SKIP_BYTES : BOOTLOADER_SKIP_BYTES + IMAGE_HEADER_SIZE]
    magic = int.from_bytes(header[0:4], "little")
    version_bytes = header[8 : 8 + FIRMWARE_VERSION_LENGTH]
    firmware_version = version_bytes.split(b"\x00", 1)[0].decode("ascii", errors="replace").strip()
    return FirmwareInfo(magic=magic, firmware_version=firmware_version)


def inspect_uf2_file(source_path: str | Path, expected_magic: int | None = None) -> FirmwareInfo:
    raw = Path(source_path).read_bytes()
    image_data, _ = uf2_to_bin_bytes(raw)
    firmware_info = extract_firmware_info(image_data)
    if expected_magic is not None and firmware_info.magic != expected_magic:
        raise Uf2Error(
            f"Unexpected firmware magic 0x{firmware_info.magic:08X}; expected 0x{expected_magic:08X}."
        )
    return firmware_info


def convert_uf2_file(
    source_path: str | Path,
    output_path: str | Path,
    expected_magic: int | None = None,
) -> BinImage:
    source = Path(source_path)
    output = Path(output_path)

    raw = source.read_bytes()
    bin_data, base_address = uf2_to_bin_bytes(raw)
    firmware_info = extract_firmware_info(bin_data)
    if expected_magic is not None and firmware_info.magic != expected_magic:
        raise Uf2Error(
            f"Unexpected firmware magic 0x{firmware_info.magic:08X}; expected 0x{expected_magic:08X}."
        )
    output.write_bytes(bin_data)

    return BinImage(
        name=output.name,
        path=output,
        data=bin_data,
        base_address=base_address,
        firmware_info=firmware_info,
    )
