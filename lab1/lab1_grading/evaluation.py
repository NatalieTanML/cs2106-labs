#!/usr/bin/env python3
import os, sys, subprocess, signal
import shutil, re, errno, math
from pathlib import Path

# https://stackoverflow.com/questions/4789837/how-to-terminate-a-python-subprocess-launched-with-shell-true
def get_output(cmd, path=".", timeout=7):  # Handle cases with infinite loops
    output = ""
    with subprocess.Popen(
        cmd,
        shell=True,
        encoding="utf8",
        stderr=subprocess.STDOUT,
        cwd=path,
        stdout=subprocess.PIPE,
        preexec_fn=os.setsid,
    ) as proc:
        try:
            output = proc.communicate(timeout=timeout)[0]
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGINT)  # kill the process tree
            output = proc.communicate()[0]
            raise
        except Exception:
            raise
    return output


def get_output_no_timeout(cmd, path="."):
    ret = subprocess.check_output(
        cmd, shell=True, encoding="utf8", stderr=subprocess.STDOUT, cwd=path
    ).strip()
    return ret


def ex(cmd):
    return subprocess.call(cmd, shell=True)


def positive_float(res):
    return not re.match(r"^\+?\d+(?:\.\d+)?$", res) is None


def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc:
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def copy_testing_files(sol_dir, workdir, ex_dir, files):
    dst_ex_dir = ex_dir.split("_")[0]

    dst_dir = os.path.join(workdir, dst_ex_dir)
    mkdir_p(dst_dir)
    src_dir = os.path.join(sol_dir, ex_dir)
    for item in files:
        src = os.path.join(src_dir, item)
        dst = os.path.join(dst_dir, item)
        try:
            shutil.copytree(src, dst)
        except OSError as exc:
            if exc.errno == errno.ENOTDIR:
                shutil.copy(src, dst)
            else:
                raise


def get_id(file1):
    file1 = file1.rsplit("/", 1)[-1]
    if "E" == file1[0].upper():
        return file1[:8]
    elif "A" == file1[0].upper():
        return file1[:9]
    return "Error"


def printable(e, workdir):
    return " ".join(str(e).replace(workdir + "/", "").strip().split())


def ex2(infile_c, workdir, sol_dir):
    exn = "ex2"
    num_qparts = 64  # just an arbitrary large number to prevent overflow
    score = [0.0] * num_qparts
    ex_dir = "ex2"
    test_list = [
        "small",
        "big",
        "ultra",
        "insert",
        "delete",
        "rotatelist",
        "reverselist",
        "resetlist",
    ]
    files = [
        "Makefile",
        "ex2.c",
        "node.h",
        *[f"{name}_test.{ext}" for name in test_list for ext in ("in", "out")],
        "reverse.c",
        "rotate.c",
    ]

    copy_testing_files(sol_dir, workdir, ex_dir, files)
    if not os.path.isfile(os.path.join(workdir, ex_dir, "node.c")):
        print(
            "[%s][%s] File 'node.c' not found" % (get_id(infile_c), exn),
            file=sys.stderr,
        )
        return 0
    cmd = "make -C %s all rotatetest reversetest > /dev/null 2>&1" % (
        os.path.join(workdir, ex_dir)
    )
    try:
        ret = ex(cmd)
    except subprocess.CalledProcessError as e:
        print(
            "[%s][%s] Compilation failed: %s"
            % (get_id(infile_c), exn, printable(e, workdir)),
            file=sys.stderr,
        )
        return 0
    if not ret:
        score[0] = 2
    else:
        print(
            "[%s][%s] Compilation failed: Errcode %s"
            % (get_id(infile_c), exn, printable(ret, workdir)),
            file=sys.stderr,
        )
        return 0

    def gen_cmd(workdir, ex_dir, ex_no, testfile):
        cmd = os.path.join(workdir, ex_dir, ex_no)
        cmd = " ".join([cmd, "<", os.path.join(workdir, ex_dir, "%s.in" % (testfile))])
        cmd = " ".join(
            [
                cmd,
                "|",
                "diff",
                os.path.join(workdir, ex_dir, "%s.out" % (testfile)),
                "-",
            ]
        )
        return cmd

    i = 1
    for name in test_list:
        cmd = gen_cmd(workdir, ex_dir, exn, f"{name}_test")
        try:
            ret = get_output(cmd, path=os.path.join(workdir, ex_dir))
        except Exception as e:
            print(
                f"[{get_id(infile_c)}][{exn}] Runtime error with '{name}' testcase. {printable(e, workdir)}",
                file=sys.stderr,
            )
            score[i] = 0
        else:
            if len(ret) > 0:
                print(
                    f"[{get_id(infile_c)}][{exn}] Wrong output with '{name}' testcase",
                    file=sys.stderr,
                )
                score[i] = 0
            else:
                score[i] = 2
        i += 1

    cmd = str(Path(workdir) / ex_dir / "reversetest")
    ret = ex(cmd)
    if ret == 0:
        score[i] = 2
    else:
        print(
            f"[{get_id(infile_c)}][{exn}] allocate memory in reversion", file=sys.stderr
        )
        score[i] = 0
    i += 1

    cmd = str(Path(workdir) / ex_dir / "rotatetest")
    ret = ex(cmd)
    if ret == 0:
        score[i] = 2
    else:
        print(
            f"[{get_id(infile_c)}][{exn}] allocate memory in rotation", file=sys.stderr
        )
        score[i] = 0
    i += 1

    return math.floor(sum(score))


def ex3(infile_c, workdir, sol_dir):
    exn = "ex3"
    num_qparts = 64
    score = [0.0] * num_qparts
    ex_dir = "ex3"
    files = [
        "Makefile",
        "node.h",
        "functions.c",
        "functions.h",
        "function_pointers.h",
        "ultra_test.in",
        "ultra_test.out",
        "big_test.in",
        "big_test.out",
        "small_test.in",
        "small_test.out",
    ]
    copy_testing_files(sol_dir, workdir, ex_dir, files)
    if not os.path.isfile(os.path.join(workdir, ex_dir, "node.c")):
        print(
            "[%s][%s] File 'node.c' not found" % (get_id(infile_c), exn),
            file=sys.stderr,
        )
        return 0
    cmd = "make -C %s ex3 > /dev/null 2>&1" % (os.path.join(workdir, ex_dir))
    try:
        ret = ex(cmd)
    except subprocess.CalledProcessError as e:
        print(
            "[%s][%s] Compilation failed: %s"
            % (get_id(infile_c), exn, printable(e, workdir)),
            file=sys.stderr,
        )
        return 0
    if not ret:
        score[0] = 2
    else:
        print(
            "[%s][%s] Compilation failed: Errcode %s"
            % (get_id(infile_c), exn, printable(ret, workdir)),
            file=sys.stderr,
        )
        return 0

    def gen_cmd(workdir, ex_dir, ex_no, testfile):
        cmd = os.path.join(workdir, ex_dir, ex_no)
        cmd = " ".join([cmd, os.path.join(workdir, ex_dir, "%s.in" % (testfile))])
        cmd = " ".join(
            [
                cmd,
                "|",
                "diff",
                os.path.join(workdir, ex_dir, "%s.out" % (testfile)),
                "-",
            ]
        )
        return cmd

    cmd = gen_cmd(workdir, ex_dir, exn, "ultra_test")
    try:
        ret = get_output(cmd, path=os.path.join(workdir, ex_dir))
    except Exception as e:
        print(
            "[%s][%s] Runtime error with 'ultra_test.in' testcase. %s"
            % (get_id(infile_c), exn, printable(e, workdir)),
            file=sys.stderr,
        )
        score[1] = 0
    else:
        if len(ret) > 2:
            print(
                "[%s][%s] Wrong output with 'ultra_test.in' testcase"
                % (get_id(infile_c), exn),
                file=sys.stderr,
            )
            score[1] = 0
        else:
            score[1] = 2

    files = [
        "function_pointers.c",
        "function_pointers.h.1",
        "fp_test.in",
        "fp_test.out",
    ]
    copy_testing_files(sol_dir, workdir, exn, files)
    ex(
        f"mv {Path(workdir) / ex_dir / 'function_pointers.h.1'}"
        f"   {Path(workdir) / ex_dir / 'function_pointers.h'}"
    )
    cmd = "make fptest -C %s > /dev/null 2>&1" % (os.path.join(workdir, ex_dir))
    try:
        ret = ex(cmd)
    except subprocess.CalledProcessError as e:
        print(
            "[%s][%s] Function pointer test. Compilation failed: %s"
            % (get_id(infile_c), exn, printable(e, workdir)),
            file=sys.stderr,
        )
        return math.floor(sum(score))
    if not ret:
        score[2] = 2
    else:
        print(
            "[%s][%s] Function pointer test. Compilation failed: Errcode %s"
            % (get_id(infile_c), exn, printable(ret, workdir)),
            file=sys.stderr,
        )
        return math.floor(sum(score))

    cmd = gen_cmd(workdir, ex_dir, "fptest", "fp_test")
    try:
        ret = get_output(cmd, path=os.path.join(workdir, ex_dir))
    except Exception as e:
        print(
            "[%s][%s] Runtime error with function pointer test using 'fp_test.in'. %s"
            % (get_id(infile_c), exn, printable(e, workdir)),
            file=sys.stderr,
        )
        score[3] = 0
    else:
        if len(ret) > 2:
            print(
                "[%s][%s] Function pointer test failed using 'fp_test.in'"
                % (get_id(infile_c), exn),
                file=sys.stderr,
            )
            score[3] = 0
        else:
            score[3] = 2

    return math.floor(sum(score))


def ex4(infile_c, workdir, sol_dir):
    exn = "ex4"
    num_qparts = 2
    score = [0.0] * num_qparts
    apps_to_check = []
    test_in = "big_test.in"
    app1 = os.path.join(workdir, "ex2", "ex2")
    if not os.path.isfile(app1):
        score[0] = 0
        print(
            "[%s][%s] Executable file 'ex2' not found" % (get_id(infile_c), exn),
            file=sys.stderr,
        )
        apps_to_check.append(-1)
    else:
        app1 = " ".join([app1, "<", os.path.join(workdir, "ex2", test_in)])
        apps_to_check.append(app1)
    app2 = os.path.join(workdir, "ex3", "ex3")
    if not os.path.isfile(app2):
        score[1] = 0
        print(
            "[%s][%s] Executable file 'ex3' not found" % (get_id(infile_c), exn),
            file=sys.stderr,
        )
        apps_to_check.append(-1)
    else:
        app2 = " ".join([app2, os.path.join(workdir, "ex3", test_in)])
        apps_to_check.append(app2)
    if not apps_to_check:
        return math.floor(sum(score))
    for it, app in enumerate(apps_to_check):
        if app == -1:
            continue
        ret = ""
        cmd = "valgrind -q %s > /dev/null" % (app)
        try:
            ret = get_output(cmd, path=workdir, timeout=10)
        except Exception as e:
            print(
                "[%s][%s] Runtime error. %s"
                % (get_id(infile_c), exn, printable(e, workdir)),
                file=sys.stderr,
            )
            return -2
        if len(ret) > 2:
            print(
                "[%s][%s] Valgrind reports memory errors/leakings for %s"
                % (get_id(infile_c), exn, printable(app, workdir)),
                file=sys.stderr,
            )
            score[it] = -1
        else:
            score[it] = 0
    return math.floor(sum(score))


def ex5(infile_c, workdir, sol_dir):
    exn = "ex5"
    num_qparts = 12
    score = [0.0] * num_qparts
    app = os.path.join(workdir, "ex5", "check_system.sh")
    if not os.path.isfile(app):
        print(
            "[%s][%s] File 'check_system.sh' not found" % (get_id(infile_c), exn),
            file=sys.stderr,
        )
        return 0
    cmd = "bash %s" % (app)
    ret = ""
    try:
        ret = get_output(cmd, path=os.path.join(workdir, "ex5"), timeout=10)
    except Exception as e:
        print(
            "[%s][%s] %s" % (get_id(infile_c), exn, printable(e, workdir)),
            file=sys.stderr,
        )
        return 0
    ret_lines = ret.splitlines()
    for line in ret_lines:  # 4 marks
        if "Hostname:" in line:
            res = line.split(":")[-1].strip()
            if len(res) > 2:
                score[0] = 0.5
            else:
                print(
                    "[%s][%s] 'Hostname' missing" % (get_id(infile_c), exn),
                    file=sys.stderr,
                )
        elif "Linux Kernel Version:" in line:
            res = line.split(":")[-1].strip()
            if len(res) > 3:
                score[1] = 0.5
            else:
                print(
                    "[%s][%s] 'Linux Kernel Version' missing" % (get_id(infile_c), exn),
                    file=sys.stderr,
                )
        elif "Total Processes:" in line:
            res = line.split(":")[-1].strip()
            if res.isdigit() and int(res) > 3:
                score[2] = 0.5
            else:
                print(
                    "[%s][%s] 'Total Processes' missing" % (get_id(infile_c), exn),
                    file=sys.stderr,
                )
        elif "User Processes:" in line:
            res = line.split(":")[-1].strip()
            if res.isdigit():
                score[3] = 0.5
            else:
                print(
                    "[%s][%s] 'User Processes' missing" % (get_id(infile_c), exn),
                    file=sys.stderr,
                )
        elif "Memory Used (%):" in line:
            res = line.split(":")[-1].strip()
            if positive_float(res):
                score[4] = 1
            else:
                print(
                    "[%s][%s] 'Memory Used (%%)' missing" % (get_id(infile_c), exn),
                    file=sys.stderr,
                )
        elif "Swap Used (%):" in line:
            res = line.split(":")[-1].strip()
            if positive_float(res):
                score[5] = 1
            else:
                print(
                    "[%s][%s] 'Swap Used (%%)' missing" % (get_id(infile_c), exn),
                    file=sys.stderr,
                )

    with open(app, "r") as appf:  # 4 marks
        for line in appf:
            if "hostname=" in line:
                if "hostname" in line or all(i in line for i in ["uname", "-n"]):
                    score[6] = 0.5
                else:
                    print(
                        "[%s][%s] Script for 'hostname' missing"
                        % (get_id(infile_c), exn),
                        file=sys.stderr,
                    )
            elif "kernel_version=" in line:
                if all(i in line for i in ["uname", "-r"]):
                    score[7] = 0.5
                else:
                    print(
                        "[%s][%s] Script for 'kernel_version' missing"
                        % (get_id(infile_c), exn),
                        file=sys.stderr,
                    )
            elif "user_process_cnt=" in line:
                if all(i in line for i in ["ps", "wc"]):
                    score[9] = 0.5
                else:
                    print(
                        "[%s][%s] Script for 'user_process_cnt' missing"
                        % (get_id(infile_c), exn),
                        file=sys.stderr,
                    )
            elif "process_cnt=" in line:
                if all(i in line for i in ["ps", "wc"]):
                    score[8] = 0.5
                else:
                    print(
                        "[%s][%s] Script for 'process_cnt' missing"
                        % (get_id(infile_c), exn),
                        file=sys.stderr,
                    )
            elif "mem_usage=" in line:
                if all(i in line for i in ["free", "awk"]):
                    score[10] = 1
                else:
                    print(
                        "[%s][%s] Script for 'mem_usage' missing"
                        % (get_id(infile_c), exn),
                        file=sys.stderr,
                    )
            elif "swap_usage=" in line:
                if all(i in line for i in ["free", "awk"]):
                    score[11] = 1
                else:
                    print(
                        "[%s][%s] Script for 'swap_usage' missing"
                        % (get_id(infile_c), exn),
                        file=sys.stderr,
                    )
    return math.floor(sum(score))


def ex6(infile_c, workdir, sol_dir):
    exn = "ex6"
    score = 0
    app = os.path.join(workdir, "ex6", "check_syscalls.sh")
    if not os.path.isfile(app):
        print(
            "[%s][%s] File 'check_syscalls.sh' not found" % (get_id(infile_c), exn),
            file=sys.stderr,
        )
        return score
    with open(app, "r") as scr:
        for line in scr:
            if not line.startswith("#"):
                if "strace" in line and any(
                    i in line for i in ["--summary", "-c", "-C"]
                ):
                    score = 4
    if score < 4:
        print("[%s][%s] Improper command" % (get_id(infile_c), exn), file=sys.stderr)
    return score
