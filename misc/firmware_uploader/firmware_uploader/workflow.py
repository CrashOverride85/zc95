from __future__ import annotations

import tempfile
from pathlib import Path
from typing import Callable

from serial.tools import list_ports

from .protocol import SerialBootloaderClient
from .uf2 import BinImage, FirmwareInfo, Uf2Error, convert_uf2_file, inspect_uf2_file


EXPECTED_FILENAMES = ("zc95.uf2", "OutputZc.uf2")
EXPECTED_MAGICS = {
    "zc95.uf2": 0x5A433935,
    "OutputZc.uf2": 0x7A363234,
}
FIRST_COMMAND = "95:05"
SECOND_COMMAND = "624:05"
FINAL_COMMAND = "95:07"

LogCallback = Callable[[str], None]
ProgressCallback = Callable[[int], None]


class WorkflowError(RuntimeError):
    """Raised for high-level workflow failures."""


def validate_selected_files(paths: list[str]) -> dict[str, Path]:
    if not paths:
        raise WorkflowError("Select at least one UF2 file.")

    selected = {Path(path).name: Path(path) for path in paths}
    extra = [name for name in selected if name not in EXPECTED_FILENAMES]

    if extra:
        raise WorkflowError(
            "Expected files named zc95.uf2 and OutputZc.uf2."
        )

    return selected


def list_serial_ports() -> list[str]:
    ports = sorted(port.device for port in list_ports.comports())
    return ports


def inspect_selected_file(path: str) -> FirmwareInfo:
    source = Path(path)
    filename = source.name
    if filename not in EXPECTED_MAGICS:
        raise WorkflowError("Expected files named zc95.uf2 and OutputZc.uf2.")
    try:
        return inspect_uf2_file(source, expected_magic=EXPECTED_MAGICS[filename])
    except Uf2Error as exc:
        raise WorkflowError(f"{filename} is not a valid firmware image: {exc}") from exc


def convert_selected_files(paths: list[str], log: LogCallback | None = None) -> list[BinImage]:
    selected = validate_selected_files(paths)
    converted: list[BinImage] = []

    temp_dir = Path(tempfile.mkdtemp(prefix="firmware_uploader_"))
    for filename in EXPECTED_FILENAMES:
        if filename not in selected:
            continue
        source = selected[filename]
        output = temp_dir / source.with_suffix(".bin").name
        if log is not None:
            log(f"Converting {source.name} to {output.name}.")
        try:
            converted.append(convert_uf2_file(source, output, expected_magic=EXPECTED_MAGICS[filename]))
        except Uf2Error as exc:
            raise WorkflowError(f"{source.name} is not a valid UF2 file: {exc}") from exc

    return converted


def _percent(current: int, total: int) -> int:
    if total <= 0:
        return 0
    return int((current / total) * 100)


def _scaled_progress(
    stage_index: int,
    stage_count: int,
    current: int,
    total: int,
) -> int:
    if stage_count <= 0:
        return 0
    stage_size = 100 / stage_count
    stage_offset = stage_index * stage_size
    return min(100, int(stage_offset + ((_percent(current, total) / 100) * stage_size)))


def run_upload_workflow(
    uf2_paths: list[str],
    port_name: str,
    log: LogCallback | None = None,
    progress: ProgressCallback | None = None,
) -> None:
    converted = convert_selected_files(uf2_paths, log=log)
    image_map = {image.name: image for image in converted}
    upload_stages: list[tuple[str, BinImage]] = []
    if "zc95.bin" in image_map:
        upload_stages.append((FIRST_COMMAND, image_map["zc95.bin"]))
    if "OutputZc.bin" in image_map:
        upload_stages.append((SECOND_COMMAND, image_map["OutputZc.bin"]))

    if progress is not None:
        progress(0)

    with SerialBootloaderClient(port_name=port_name) as client:
        if log is not None:
            log(f"Opened serial port {port_name} at 115200 8N1.")
            log("Switch on the device now.")

        client.wait_for_syn(timeout_seconds=60.0, log=log)

        for stage_index, (command, image) in enumerate(upload_stages):
            client.send_framed_message(command, log=log)
            client.send_xmodem_crc(
                image.upload_data,
                progress=lambda current, total, stage_index=stage_index: progress(
                    _scaled_progress(stage_index, len(upload_stages), current, total)
                )
                if progress is not None
                else None,
                log=log,
            )

        client.send_framed_message(FINAL_COMMAND, log=log)

    if progress is not None:
        progress(100)
    if log is not None:
        log("Firmware upload completed successfully.")
