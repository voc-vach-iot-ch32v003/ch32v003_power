import os
import re
import subprocess
import sys
import time
from SCons.Script import DefaultEnvironment

os.environ["PLATFORMIO_FORCE_ANSI"] = "true"

env = DefaultEnvironment()
current_env = env["PIOENV"]

# Regex loại bỏ toàn bộ mã ANSI Escape Code
ANSI_ESCAPE = re.compile(r"\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])")


# -----------------------------------------------------------------------------
# Hàm xử lý SDI Monitor
# -----------------------------------------------------------------------------
def run_sdi_monitor(target=None, source=None, env=None):
    pkg_dir = (
        env.PioPlatform().get_package_dir("tool-minichlink") if env else None
    )
    if pkg_dir:
        minichlink_bin = os.path.join(pkg_dir, "minichlink")
    else:
        minichlink_bin = os.path.expanduser(
            "~/.platformio/packages/tool-minichlink/minichlink"
        )

    cmd = (
        [minichlink_bin, "-T"]
        if os.path.exists(minichlink_bin)
        else ["minichlink", "-T"]
    )

    ignore_keywords = [
        "Found WCH Link",
        "WCH Programmer",
        "Detected CH32V003",
        "Flash Storage",
        "Part UUID",
        "Part Type",
        "Read protection",
        "Interface Setup",
    ]

    print("\n---> Starting SDI Monitor (Press Ctrl+C to stop)...\n")

    try:
        proc = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            encoding="utf-8",
            errors="ignore",
        )

        for line in iter(proc.stdout.readline, ""):
            # 1. Bỏ mã ANSI
            clean = ANSI_ESCAPE.sub("", line)

            # 2. Bỏ MỌI ký tự không in được (zero-width space, BOM, control char...)
            #    giữ lại '\n' để không phá cấu trúc dòng (thực ra không cần vì đã readline theo dòng)
            clean = "".join(ch for ch in clean if ch.isprintable() or ch in "\n")

            # 3. Xóa khoảng trắng để kiểm tra nội dung "thực"
            clean_stripped = re.sub(r"\s+", "", clean)

            # 4. Bỏ qua dòng rỗng
            if not clean_stripped:
                continue

            # 5. Bỏ qua dòng chỉ chứa rác quen thuộc (thêm cả các ký tự spinner phổ biến nếu cần)
            if re.fullmatch(r"[eyE_\.\u2026\|/\\\-]+", clean_stripped, re.IGNORECASE):
                continue

            # 6. Bỏ qua theo từ khóa
            if any(kw in clean for kw in ignore_keywords):
                continue

            # 7. In dòng sạch
            final_text = clean.strip()
            if not final_text:
                continue
            now = time.strftime("%H:%M:%S")
            ms = int((time.time() * 1000) % 1000)
            print(f"[{now}.{ms:03d}] {final_text}")
            sys.stdout.flush()

    except KeyboardInterrupt:
        print("\n---> Stopped SDI Monitor.")
    except Exception as e:
        print(f"\n---> Error launching minichlink: {e}")


# -----------------------------------------------------------------------------
# Target MỚI
# -----------------------------------------------------------------------------
env.AddCustomTarget(
    name="sdi_printf_monitor_ms",
    title="Monitor SDI Printf with Milliseconds",
    dependencies=[],
    actions=[run_sdi_monitor],
)

# -----------------------------------------------------------------------------
# Target GỐC CỦA BẠN
# -----------------------------------------------------------------------------
env.AddCustomTarget(
    name="upload_and_sdi_monitor",
    title="Upload and Monitor SDI",
    dependencies=[],
    actions=[
        f"pio run -t upload -e {current_env}",
        # f"pio run -t sdi_printf_monitor -e {current_env}",
        run_sdi_monitor
    ],
)
