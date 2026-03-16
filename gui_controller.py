import tkinter as tk
from tkinter import ttk, messagebox
import subprocess
import threading
import sys
import os
import re

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
        self.running = False
        self.available_ports = []

        self.setup_ui()
        self.refresh_ports()

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

        # Port Selection
        ttk.Label(main_frame, text="Output Port:").grid(row=2, column=0, sticky=tk.W)
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(main_frame, textvariable=self.port_var, state="readonly")
        self.port_combo.grid(row=2, column=1, sticky=(tk.W, tk.E))

        ttk.Button(main_frame, text="Refresh", command=self.refresh_ports).grid(row=2, column=2)

        # Connection status and Start/Stop
        self.status_var = tk.StringVar(value="Status: Disconnected")
        ttk.Label(main_frame, textvariable=self.status_var).grid(row=3, column=0, columnspan=3, pady=5)

        self.start_btn = ttk.Button(main_frame, text="Start MIDI", command=self.toggle_midi)
        self.start_btn.grid(row=4, column=0, columnspan=3, pady=5)

        ttk.Button(main_frame, text="Quit", command=self.quit).grid(row=5, column=0, columnspan=3, pady=10)

        info_frame = ttk.LabelFrame(main_frame, text="Instructions", padding="5")
        info_frame.grid(row=6, column=0, columnspan=3, pady=10, sticky=(tk.W, tk.E))

        instructions = (
            "1. Run 'make' to build the C++ logic.\n"
            "2. Ensure TiMidity is running ('timidity -iA').\n"
            "3. Select TiMidity port from dropdown.\n"
            "4. Click 'Start MIDI'."
        )
        ttk.Label(info_frame, text=instructions, justify=tk.LEFT).grid(row=0, column=0)

    def refresh_ports(self):
        self.available_ports = []
        try:
            # Try to list ports using aconnect -o
            output = subprocess.check_output(['aconnect', '-o'], text=True)
            # Parse output: client 128: 'TiMidity' [type=user,pid=13410]
            #                    0 'TiMidity port 0 '
            current_client = None
            for line in output.split('\n'):
                client_match = re.search(r'client (\d+): \'(.*?)\'', line)
                if client_match:
                    current_client = (client_match.group(1), client_match.group(2))
                    continue

                port_match = re.search(r'^\s+(\d+) \'(.*?)\'', line)
                if port_match and current_client:
                    port_id = port_match.group(1)
                    port_name = port_match.group(2)
                    display_name = f"{current_client[0]}:{port_id} ({current_client[1]} - {port_name})"
                    self.available_ports.append(display_name)
        except:
            pass

        if not self.available_ports:
            self.available_ports = ["128:0 (Default TiMidity)"]

        self.port_combo['values'] = self.available_ports
        if self.available_ports:
            # Try to select TiMidity by default
            idx = 0
            for i, p in enumerate(self.available_ports):
                if "TiMidity" in p:
                    idx = i
                    break
            self.port_combo.current(idx)

    def toggle_midi(self):
        if not self.running:
            self.start_midi()
        else:
            self.stop_midi()

    def start_midi(self):
        try:
            # Start Bridge
            self.bridge = subprocess.Popen(
                [self.bridge_path],
                stdin=subprocess.PIPE,
                stdout=sys.stdout,
                stderr=sys.stderr,
                text=True,
                bufsize=1
            )

            # Extract port if possible
            port_str = self.port_var.get()
            m = re.match(r'(\d+):(\d+)', port_str)
            if m:
                client_id = m.group(1)
                port_id = m.group(2)
                # We can't easily tell the bridge which port to connect to via args yet
                # but we can use aconnect manually from Python
                threading.Timer(1.0, lambda: subprocess.run(['aconnect', 'PWM MIDI Bridge', f'{client_id}:{port_id}'])).start()

            self.running = True
            self.start_btn.config(text="Stop MIDI")
            self.status_var.set(f"Status: Running (Port {port_str})")

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
                self.bridge.stdin.write("q\n")
                self.bridge.stdin.flush()
            except: pass
            self.bridge.terminate()

        self.bridge = None
        self.start_btn.config(text="Start MIDI")
        self.status_var.set("Status: Disconnected")

    def update_error(self, val):
        v = int(float(val))
        self.error_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"e{v}\n")
                self.bridge.stdin.flush()
            except: pass

    def update_base(self, val):
        v = int(float(val))
        self.base_val.config(text=str(v))
        if self.bridge and self.bridge.stdin:
            try:
                self.bridge.stdin.write(f"b{v}\n")
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
