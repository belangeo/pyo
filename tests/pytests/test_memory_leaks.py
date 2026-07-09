import gc

import pytest

from pyo import CallAfter, Metro, TrigFunc
from utilities import Start


class _LeakToken:
    live_count = 0

    def __init__(self):
        type(self).live_count += 1

    def __del__(self):
        type(self).live_count -= 1


def _collect_garbage():
    for _ in range(3):
        gc.collect()


def _assert_no_callback_leak(work, iterations=25):
    _LeakToken.live_count = 0

    for _ in range(iterations):
        work()
        _collect_garbage()

    assert _LeakToken.live_count == 0


@pytest.mark.usefixtures("audio_server")
class TestMemoryLeaks:
    def test_callafter_callbacks_do_not_leak_references(self, audio_server):
        period = 4.0 / audio_server.getSamplingRate()

        def callback_noarg():
            return _LeakToken()

        def callback_arg(value):
            return _LeakToken()

        with Start(audio_server):
            def work():
                noarg = CallAfter(callback_noarg, period).play()
                witharg = CallAfter(callback_arg, period, 1).play()
                audio_server.process()
                audio_server.process()
                del noarg, witharg

            _assert_no_callback_leak(work)

        _collect_garbage()
        assert audio_server.getNumberOfStreams() == 0

    def test_trigfunc_callbacks_do_not_leak_references(self, audio_server):
        period = 4.0 / audio_server.getSamplingRate()

        def callback_noarg():
            return _LeakToken()

        def callback_arg(value):
            return _LeakToken()

        with Start(audio_server):
            def work():
                metro = Metro(time=period).play()
                noarg = TrigFunc(metro, callback_noarg)
                witharg = TrigFunc(metro, callback_arg, 1)
                audio_server.process()
                audio_server.process()
                del witharg, noarg, metro

            _assert_no_callback_leak(work)

        _collect_garbage()
        assert audio_server.getNumberOfStreams() == 0
