import tkinter as tk
from tkinter import messagebox
from tkinter.scrolledtext import ScrolledText
import subprocess
from threading import Thread
import serial.tools.list_ports
import time

def update_com_ports():
    """연결된 COM 포트를 주기적으로 업데이트."""
    while True:
        ports = serial.tools.list_ports.comports()
        port_names = [port.device for port in ports]
        com_label.config(text=f"Available COM Ports: {', '.join(port_names)}" if port_names else "No COM Ports detected")
        time.sleep(2)  # 2초 간격으로 업데이트

def execute_command():
    """명령어 실행."""
    com_port = com_entry.get()
    if not com_port:
        output_text.insert(tk.END, "Error: Input COM Port!\n")
        return

    # COM 포트를 따옴표로 감싸기
    com_port_quoted = f'"{com_port}"'

    # 명령어 구성
    command = [
        "upload_image_tool_windows.exe",
        "\\bin",
        com_port_quoted,
        "{board}",
        "Disable",
        "Disable",
        "1500000"
    ]

    def run_and_capture():
        try:
            process = subprocess.Popen(
                " ".join(command),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                shell=True,
                bufsize=1
            )

            # stdout 실시간 읽기
            for char in iter(lambda: process.stdout.read(1), ''):
                output_text.insert(tk.END, char)
                output_text.see(tk.END)

            # stderr 처리
            for line in process.stderr:
                output_text.insert(tk.END, f"Error: {line}")
                output_text.see(tk.END)

            process.wait()
            output_text.insert(tk.END, "\nCommand execution completed.\n")
        except Exception as e:
            output_text.insert(tk.END, f"Error: {str(e)}\n")

    # 별도의 쓰레드에서 실행
    thread = Thread(target=run_and_capture)
    thread.start()

# GUI 설정
root = tk.Tk()
root.title("5G Deauther Flasher")
root.geometry("600x500")  # 창 크기 지정
root.config(bg="#f0f0f0")  # 배경 색상 설정

# 폰트 설정
font_style = ("Arial", 12)

# COM 포트 라벨
com_label = tk.Label(root, text="Available COM Ports: Detecting...", fg="blue", font=("Arial", 14, "bold"), bg="#f0f0f0")
com_label.pack(pady=10)

# COM 포트 입력
tk.Label(root, text="Enter COM Port (e.g., COM5):", font=font_style, bg="#f0f0f0").pack(pady=10)
com_entry = tk.Entry(root, font=font_style, width=25)
com_entry.pack(pady=5)

# 실행 버튼 스타일
execute_button = tk.Button(root, text="Execute", command=execute_command, font=font_style, bg="#4CAF50", fg="white", relief="flat", width=20)
execute_button.pack(pady=20)

# 출력 텍스트 박스 스타일
output_text = ScrolledText(root, wrap=tk.WORD, height=10, width=70, font=("Courier New", 10), bg="#f4f4f4", fg="black", bd=2, relief="sunken")
output_text.pack(pady=10)

# COM 포트 업데이트 쓰레드 시작
com_thread = Thread(target=update_com_ports, daemon=True)
com_thread.start()

# GUI 실행
root.mainloop()
