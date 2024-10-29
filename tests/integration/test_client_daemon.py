import pytest
import subprocess
import signal
import time
import string
import random
import shutil
import os

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


@pytest.fixture()
def daemon_preloaded_tag():
    # Setup the tagfile
    os.mkdir(NAV_ROOT)
    with open(f"{NAV_ROOT}/tags", "w") as tagfile:
        tagfile.write("test=/tmp/\n")
        tagfile.write("\n")

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
    assert client.stdout.strip() == "OK"

    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "unregister"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"


def test_tags_add_get_show_delete(daemon):
    """
    Test tagging: add, get, delete and tagfile.
    """

    pid = "123456"

    # Register with daemon
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "register"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Add test --> /tmp/
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "add", "test", "/tmp/"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Get test --> /tmp/
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "get", "test"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "/tmp/"

    # Show tags
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "show"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "test --> /tmp/"

    # Check tagfile
    with open(f"{NAV_ROOT}/tags", "r") as tagfile:
        line = tagfile.readline()
        assert line == "test=/tmp/\n"

        line = tagfile.readline()
        assert line == "\n", "Missing newline at end of file"
    
    # Delete test --> /tmp/
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "delete", "test"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Check that test is deleted
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "get", "test"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "BAD"

    # Check tagfile
    with open(f"{NAV_ROOT}/tags") as tagfile:
        line = tagfile.readline()
        assert line == "\n", "Missing newline at end of file"

    # Unregister with daemon
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "unregister"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"


def test_preloaded_tagfile(daemon_preloaded_tag):
    """
    Test tagfiles that have preloaded tags
    """
    pid = "123456"

    # Register with daemon
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "register"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Get test --> /tmp/
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "get", "test"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "/tmp/"

    # Show tags
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "show"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "test --> /tmp/"

    # Unregister with daemon
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "unregister"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"



def test_action_stack(daemon):
    """
    Test the action stack: push, pop, and empty pop.
    """
    pid = "123456"

    # Register with daemon
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "register"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Push /tmp/
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "push", "/tmp/"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Push /home/
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "push", "/home/"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"

    # Pop 
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "pop"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "/home/"

    # Pop 
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "pop"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "/tmp/"

    # Empty pop 
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "pop"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "BAD"

    # Unregister with daemon
    client = subprocess.run([CLIENT_PATH, "-d", NAV_ROOT, pid,
                             "unregister"],
                            capture_output=True, text=True)

    assert client.returncode == 0
    assert client.stdout.strip() == "OK"