import pytest
from pyo import Server

@pytest.fixture
def audio_server():
    # setup
    server = Server(sr=48000, buffersize=512, audio="manual").boot()

    yield server

    # teardown
    server.shutdown()
