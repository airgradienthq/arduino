import subprocess
import os

def get_git_version():
    try:
        version = subprocess.check_output(["git", "describe", "--tags", "--always", "--dirty"]).decode("utf-8").strip()
        return version
    except Exception as e:
        print("Could not get git version:", e)
        return "unknown"

git_version = get_git_version()
# git_version = "0.8.7-1-g118f259-dirty"
print("GIT_VERSION:", git_version)

# เพิ่ม GIT_VERSION เป็น build flag
Import("env")
env.Append(CPPDEFINES=[("GIT_VERSION", '\\"{}\\"'.format(git_version))])
