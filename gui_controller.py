import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import threading
import sys
import os

class MIDIController:
    def __init__(self, root):
        self.root = root
        self.root.title("PWM MIDI Controller")

        # Determine path to bridge
        base_dir = os.path.dirname(os.path.abspath(__file__))
        self.bridge_path = os.path.join(base_dir, 'tests', 'gui_bridge')

        if not os.path.exists(self.bridge_path):
            messagebox.showerror("Error", f"Bridge binary not found at {self.bridge_path}.\nPlease run 'make' first.")
            sys.exit(1)

        self.bridge = None
        self.timidity = None
        self.running = False

        self.setup_ui()

    def setup_ui(self):
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))

        # Controls
        ttk.Label(main_frame, text="Error Value (0-127)").grid(row=0, column=0, sticky=tk.W)
        self.error_scale = ttk.Scale(main_frame, from_=0, to=127, orient=tk.HORIZONTAL, command=self.update_error)
        self.error_scale.grid(row=0, column=1, sticky=(tk.W, tk.E))
        self.error_val = ttk.Label(main_frame, text="0")
        self.error_val.grid(row=0, column=2)

        ttk.Label(main_frame, text="Base Frequency (MIDI Note)").grid(row=1, column=0, sticky=tk.W)
        self.base_scale = ttk.Scale(main_frame, from_=48, to=72, orient=tk.HORIZONTAL, command=self.update_base)
        self.base_scale.grid(row=1, column=1, sticky=(tk.W, tk.E))
        self.base_val = ttk.Label(main_frame, text="60")
        self.base_val.grid(row=1, column=2)
        self.base_scale.set(60)

        # Connection status and Start/Stop
        self.status_var = tk.StringVar(value="Status: Disconnected")
        ttk.Label(main_frame, textvariable=self.status_var).grid(row=2, column=0, columnspan=2, pady=5)

        self.start_btn = ttk.Button(main_frame, text="Start MIDI", command=self.toggle_midi)
        self.start_btn.grid(row=3, column=0, columnspan=3, pady=5)

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=4, column=0, columnspan=3, pady=10)

        info_frame = ttk.LabelFrame(main_frame, text="Instructions", padding="5")
        info_frame.grid(row=5, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))

        instructions = (
            "1. Run 'make' to build the C++ logic.\n"
            "2. Click 'Start MIDI' to launch the backend and TiMidity.\n"
            "3. Use sliders to adjust parameters in real-time.\n\n"
            "Note: This app launches TiMidity as a local subprocess."
        )
        ttk.Label(info_frame, text=instructions, justify=tk.LEFT).grid(row=0, column=0)

    def toggle_midi(self):
        if not self.running:
            self.start_midi()
        else:
            self.stop_midi()

    def start_midi(self):
        try:
            # 1. Start TiMidity in stdin mode
            # -iA: ALSA sequencer interface (often needed for daemon mode, but here we pipe to stdin)
            # -Os: Output to system sound (ALSA/OSS)
            # -: Read from stdin
            self.timidity = subprocess.Popen(
                ['timidity', '-iA', '-Os', '-'],
                stdin=subprocess.PIPE,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL
            )

            # 2. Start Bridge
            self.bridge = subprocess.Popen(
                [self.bridge_path],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=sys.stderr,
                bufsize=0
            )

            self.running = True
            self.start_btn.config(text="Stop MIDI")
            self.status_var.set("Status: Running (TiMidity connected)")

            # 3. Start forwarding thread
            self.forward_thread = threading.Thread(target=self.forward_midi, daemon=True)
            self.forward_thread.start()

            # Sync initial values
            self.update_error(self.error_scale.get())
            self.update_base(self.base_scale.get())

        except Exception as e:
            messagebox.showerror("Error", f"Failed to start MIDI: {e}")
            self.stop_midi()

    def stop_midi(self):
        self.running = False
        if self.bridge:
            try:
                self.bridge.stdin.write(b"q\n")
                self.bridge.stdin.flush()
            except: pass
            self.bridge.terminate()
        if self.timidity:
            self.timidity.terminate()

        self.bridge = None
        self.timidity = None
        self.start_btn.config(text="Start MIDI")
        self.status_var.set("Status: Disconnected")

    def forward_midi(self):
        try:
            while self.running:
                data = self.bridge.stdout.read(1)
                if not data:
                    break
                if self.timidity and self.timidity.stdin:
                    self.timidity.stdin.write(data)
                    self.timidity.stdin.flush()
        except Exception as e:
            if self.running:
                print(f"Forwarding error: {e}", file=sys.stderr)

    def update_error(self, val):
        v = int(float(val))
        self.error_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"e{v}\n".encode())
                self.bridge.stdin.flush()
            except: pass

    def update_base(self, val):
        v = int(float(val))
        self.base_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"b{v}\n".encode())
                self.bridge.stdin.flush()
            except: pass

    def quit(self):
        self.stop_midi()
        self.root.destroy()

if __name__ == "__main__":
    root = tk.Tk()
    app = MIDIController(root)
    root.protocol("WM_DELETE_WINDOW", app.quit)
    root.mainloop()
