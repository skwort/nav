import pytest
import subprocess
import signal
import time
import string
import random
import shutil

DAEMON_TARGET = "daemon"
CLIENT_TARGET = "client"

DAEMON_PATH = "./build/daemon"
CLIENT_PATH = "./build/client"

NAV_ROOT = "/tmp/nav-" + "".join(random.choices(string.ascii_letters, k=6))


def make_clean():
    result = subprocess.run(['make', 'clean'], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError("Build failed:\n" + result.stderr)
    return


@pytest.fixture(scope="session", autouse=True)
def build_all():
    make_clean()
    result = subprocess.run(['make', 'all'], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError("Build failed:\n" + result.stderr)
    return


@pytest.fixture()
def daemon():
    # Start the daemon process in the background
    process = subprocess.Popen([DAEMON_PATH, "-d", NAV_ROOT],
                               stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE)

    time.sleep(0.001)
    if process.poll() is not None:
        raise RuntimeError("Daemon launch failed:\n" +
                           process.stderr.read().decode())

    # Yield control back to the test
    yield process

    # Cleanup the process when finished testing
    process.send_signal(signal.SIGINT)
    try:
        process.wait(timeout=5)
    except subprocess.TimeoutExpired:
        process.kill()

    shutil.rmtree(NAV_ROOT)


def test_unregister_not_registered(daemon):
    """
    Test unregistering an unregistered client. This should fail with a non-zero
    return value.
    """
    pid = "123456"
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "unregister"],
                            capture_output=True, text=True)

    assert client.returncode == 1


def test_register_unregister(daemon):
    """
    Test registering and unregistering a client with the daemon.
    """
    pid = "123456"
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "register"],
                            capture_output=True, text=True)

    assert client.returncode == 0

    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "unregister"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"
