from __future__ import annotations

import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

from .workflow import (
    EXPECTED_FILENAMES,
    WorkflowError,
    inspect_selected_file,
    list_serial_ports,
    run_upload_workflow,
)


class FirmwareUploaderApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("Firmware Uploader")
        self.root.geometry("760x620")

        self.selected_files: dict[str, str] = {}
        self.events: queue.Queue[tuple[str, object]] = queue.Queue()
        self.worker: threading.Thread | None = None

        self.zc95_var = tk.StringVar()
        self.output_var = tk.StringVar()
        self.zc95_version_var = tk.StringVar(value="Version: -")
        self.output_version_var = tk.StringVar(value="Version: -")
        self.port_var = tk.StringVar()
        self.progress_var = tk.IntVar(value=0)
        self.status_var = tk.StringVar(value="Select one or both UF2 files.")

        self._build_ui()
        self._refresh_ports()
        self.root.after(100, self._drain_events)

    def _build_ui(self) -> None:
        frame = ttk.Frame(self.root, padding=16)
        frame.pack(fill=tk.BOTH, expand=True)
        frame.columnconfigure(1, weight=1)
        frame.rowconfigure(5, weight=1)

        ttk.Label(frame, text="zc95.uf2").grid(row=0, column=0, sticky="w", pady=4)
        ttk.Entry(frame, textvariable=self.zc95_var, state="readonly").grid(
            row=0, column=1, sticky="ew", padx=8, pady=4
        )
        self.zc95_button = ttk.Button(frame, text="Select", command=lambda: self._pick_file("zc95.uf2"))
        self.zc95_button.grid(
            row=0, column=2, pady=4
        )
        ttk.Label(frame, textvariable=self.zc95_version_var).grid(
            row=1, column=1, columnspan=2, sticky="w", padx=8, pady=(0, 8)
        )

        ttk.Label(frame, text="OutputZc.uf2").grid(row=2, column=0, sticky="w", pady=4)
        ttk.Entry(frame, textvariable=self.output_var, state="readonly").grid(
            row=2, column=1, sticky="ew", padx=8, pady=4
        )
        self.output_button = ttk.Button(
            frame,
            text="Select",
            command=lambda: self._pick_file("OutputZc.uf2"),
        )
        self.output_button.grid(row=2, column=2, pady=4)
        ttk.Label(frame, textvariable=self.output_version_var).grid(
            row=3, column=1, columnspan=2, sticky="w", padx=8, pady=(0, 8)
        )

        ttk.Label(frame, text="Serial Port").grid(row=4, column=0, sticky="w", pady=12)
        self.port_combo = ttk.Combobox(frame, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=4, column=1, sticky="ew", padx=8, pady=12)
        self.refresh_button = ttk.Button(frame, text="Refresh", command=self._refresh_ports)
        self.refresh_button.grid(row=4, column=2, pady=12)

        self.start_button = ttk.Button(frame, text="Upload", command=self._start_upload)
        self.start_button.grid(row=5, column=2, sticky="e")

        progress_frame = ttk.Frame(frame)
        progress_frame.grid(row=6, column=0, columnspan=3, sticky="ew", pady=(16, 8))
        progress_frame.columnconfigure(0, weight=1)
        self.progress_bar = ttk.Progressbar(
            progress_frame,
            maximum=100,
            variable=self.progress_var,
        )
        self.progress_bar.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        self.progress_label = ttk.Label(progress_frame, text="0%")
        self.progress_label.grid(row=0, column=1, sticky="e")

        ttk.Label(frame, textvariable=self.status_var).grid(row=7, column=0, columnspan=3, sticky="w")

        self.log_text = tk.Text(frame, height=10, state="disabled", wrap="word")
        self.log_text.grid(row=8, column=0, columnspan=3, sticky="nsew", pady=(8, 0))
        frame.rowconfigure(8, weight=1)

    def _pick_file(self, expected_name: str) -> None:
        path = filedialog.askopenfilename(
            title=f"Select {expected_name}",
            filetypes=[("UF2 files", "*.uf2"), ("All files", "*.*")],
        )
        if not path:
            return

        if Path(path).name != expected_name:
            messagebox.showerror("Invalid file", f"Selected file must be named {expected_name}.")
            return

        try:
            firmware_info = inspect_selected_file(path)
        except WorkflowError as exc:
            messagebox.showerror("Invalid file", str(exc))
            return

        self.selected_files[expected_name] = path
        if expected_name == "zc95.uf2":
            self.zc95_var.set(path)
            self.zc95_version_var.set(f"Version: {firmware_info.firmware_version or '(blank)'}")
        else:
            self.output_var.set(path)
            self.output_version_var.set(f"Version: {firmware_info.firmware_version or '(blank)'}")

        self.status_var.set("Select a serial port and start the upload.")

    def _refresh_ports(self) -> None:
        ports = list_serial_ports()
        self.port_combo["values"] = ports
        if ports:
            if self.port_var.get() not in ports:
                self.port_var.set(ports[0])
            self._append_log(f"Detected serial ports: {', '.join(ports)}")
        else:
            self.port_var.set("")
            self._append_log("No serial ports detected.")

    def _set_busy(self, busy: bool) -> None:
        state = "disabled" if busy else "normal"
        combo_state = "disabled" if busy else "readonly"
        self.start_button.configure(state=state)
        self.port_combo.configure(state=combo_state)
        self.zc95_button.configure(state=state)
        self.output_button.configure(state=state)
        self.refresh_button.configure(state=state)

    def _start_upload(self) -> None:
        if self.worker is not None and self.worker.is_alive():
            return

        if not self.selected_files:
            messagebox.showerror("Missing files", "Select at least one of zc95.uf2 or OutputZc.uf2.")
            return

        if not self.port_var.get():
            messagebox.showerror("Missing serial port", "Select a serial port.")
            return

        self.progress_var.set(0)
        self.progress_label.configure(text="0%")
        self.status_var.set("Waiting for the device to power up.")
        self._append_log("Preparing upload workflow.")
        self._set_busy(True)

        uf2_paths = [self.selected_files[name] for name in EXPECTED_FILENAMES if name in self.selected_files]
        port_name = self.port_var.get()
        self.worker = threading.Thread(
            target=self._run_upload,
            args=(uf2_paths, port_name),
            daemon=True,
        )
        self.worker.start()

    def _run_upload(self, uf2_paths: list[str], port_name: str) -> None:
        try:
            run_upload_workflow(
                uf2_paths=uf2_paths,
                port_name=port_name,
                log=lambda message: self.events.put(("log", message)),
                progress=lambda value: self.events.put(("progress", value)),
            )
        except Exception as exc:
            self.events.put(("error", exc))
        else:
            self.events.put(("success", None))

    def _drain_events(self) -> None:
        should_reschedule = True
        try:
            while True:
                event_type, payload = self.events.get_nowait()
                if event_type == "log":
                    message = str(payload)
                    self._append_log(message)
                    self._update_status_from_log(message)
                elif event_type == "progress":
                    value = int(payload)
                    self.progress_var.set(value)
                    self.progress_label.configure(text=f"{value}%")
                elif event_type == "error":
                    self._set_busy(False)
                    self.status_var.set("Upload failed.")
                    message = self._format_error(payload)
                    self._append_log(message)
                    messagebox.showerror("Upload failed", message)
                elif event_type == "success":
                    self._set_busy(False)
                    self.status_var.set("Upload complete.")
                    self._append_log("Upload finished successfully.")
                    messagebox.showinfo("Success", "Firmware upload completed successfully.")
                    should_reschedule = False
                    self.root.destroy()
        except queue.Empty:
            pass
        finally:
            if should_reschedule and self.root.winfo_exists():
                self.root.after(100, self._drain_events)

    def _append_log(self, message: str) -> None:
        self.log_text.configure(state="normal")
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")
        self.log_text.configure(state="disabled")

    def _update_status_from_log(self, message: str) -> None:
        if message.startswith("Device detected."):
            self.status_var.set("Device detected. Uploading firmware.")
        elif message.startswith("Sent command 95:05."):
            self.status_var.set("Uploading zc95 firmware.")
        elif message.startswith("Sent command 624:05."):
            self.status_var.set("Uploading OutputZc firmware.")
        elif message.startswith("Sent command 95:07."):
            self.status_var.set("Finalising upload.")

    @staticmethod
    def _format_error(error: object) -> str:
        if isinstance(error, (WorkflowError, ValueError)):
            return str(error)
        return f"Unexpected error: {error}"


def main() -> None:
    root = tk.Tk()
    app = FirmwareUploaderApp(root)
    root.minsize(680, 560)
    root.mainloop()


if __name__ == "__main__":
    main()
