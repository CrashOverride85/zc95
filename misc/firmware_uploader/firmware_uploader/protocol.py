from __future__ import annotations

import io
import time
from dataclasses import dataclass
from typing import Callable

import serial
from xmodem import XMODEM


STX = 0x02
ETX = 0x03
ACK = 0x06
SYN = 0x16
XMODEM_BLOCK_SIZE = 128

LogCallback = Callable[[str], None]
ProgressCallback = Callable[[int, int], None]


class ProtocolError(RuntimeError):
    """Raised for upload protocol failures."""


@dataclass
class SerialBootloaderClient:
    port_name: str
    baudrate: int = 115200
    timeout: float = 1.0

    def __post_init__(self) -> None:
        self.serial_port: serial.Serial | None = None

    def __enter__(self) -> "SerialBootloaderClient":
        self.open()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.close()

    def open(self) -> None:
        self.serial_port = serial.Serial(
            port=self.port_name,
            baudrate=self.baudrate,
            bytesize=serial.EIGHTBITS,
            parity=serial.PARITY_NONE,
            stopbits=serial.STOPBITS_ONE,
            timeout=self.timeout,
            write_timeout=self.timeout,
        )
        self.serial_port.reset_input_buffer()
        self.serial_port.reset_output_buffer()

    def close(self) -> None:
        if self.serial_port is not None and self.serial_port.is_open:
            self.serial_port.close()

    @property
    def port(self) -> serial.Serial:
        if self.serial_port is None:
            raise ProtocolError("Serial port is not open.")
        return self.serial_port

    def wait_for_syn(self, timeout_seconds: float, log: LogCallback | None = None) -> None:
        deadline = time.monotonic() + timeout_seconds
        if log is not None:
            log("Waiting for device power-up and SYN byte.")

        while time.monotonic() < deadline:
            received = self.port.read(1)
            if not received:
                continue
            if received[0] == SYN:
                self.port.write(bytes([ACK]))
                self.port.flush()
                time.sleep(0.1)
                if log is not None:
                    log("Device detected. ACK sent; bootloader mode entered.")
                return

        raise ProtocolError("Timed out waiting for SYN from the device.")

    def send_framed_message(self, message: str, log: LogCallback | None = None) -> None:
        # Discard any bytes from the previous stage so XMODEM start only reacts
        # to a fresh receiver request sent after this command frame.
        self.port.reset_input_buffer()
        frame = bytes([STX]) + message.encode("ascii") + bytes([ETX])
        self.port.write(frame)
        self.port.flush()
        time.sleep(0.1)
        if log is not None:
            log(f"Sent command {message}.")

    def send_xmodem_crc(
        self,
        payload: bytes,
        progress: ProgressCallback | None = None,
        log: LogCallback | None = None,
        start_timeout_seconds: float = 15.0,
        retry_limit: int = 10,
    ) -> None:
        if log is not None:
            log("Starting XMODEM/CRC transfer.")
        total_blocks = max(1, (len(payload) + XMODEM_BLOCK_SIZE - 1) // XMODEM_BLOCK_SIZE)
        if progress is not None:
            progress(0, total_blocks)

        stream = io.BytesIO(payload)

        def getc(size: int, timeout: float = start_timeout_seconds) -> bytes | None:
            previous_timeout = self.port.timeout
            self.port.timeout = timeout
            try:
                data = self.port.read(size)
            finally:
                self.port.timeout = previous_timeout
            return data or None

        def putc(data: bytes, timeout: float = start_timeout_seconds) -> int | None:
            previous_write_timeout = self.port.write_timeout
            self.port.write_timeout = timeout
            try:
                written = self.port.write(data)
                self.port.flush()
            finally:
                self.port.write_timeout = previous_write_timeout
            return written

        def callback(total_packets: int, success_count: int, error_count: int) -> None:
            if progress is not None:
                progress(min(success_count, total_blocks), total_blocks)
            if log is not None and error_count:
                log(
                    f"XMODEM retry reported by library: "
                    f"{success_count}/{total_packets} blocks acknowledged, "
                    f"errors={error_count}."
                )

        modem = XMODEM(getc, putc, mode="xmodem")
        success = modem.send(
            stream,
            retry=retry_limit,
            timeout=start_timeout_seconds,
            callback=callback,
        )
        if not success:
            raise ProtocolError("XMODEM transfer failed.")

        if progress is not None:
            progress(total_blocks, total_blocks)
        if log is not None:
            log("XMODEM/CRC transfer complete.")
