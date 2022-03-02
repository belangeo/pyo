import os
import math
import pytest
from pyo import *

@pytest.mark.usefixtures("audio_server")
class TestConversionFunctions:

    def test_midiToHz(self):
        # test with a number as arg
        in_num = 48
        out_num = midiToHz(in_num)
        assert isinstance(out_num, (int, float))
        assert math.isclose(out_num, 130.8128, abs_tol=0.001)

        # test with a tuple as arg
        in_tuple = (48, 52, 55)
        out_tuple = midiToHz(in_tuple)
        assert type(out_tuple) is tuple
        assert math.isclose(out_tuple[0], 130.8128, abs_tol=0.001)
        assert math.isclose(out_tuple[1], 164.8138, abs_tol=0.001)
        assert math.isclose(out_tuple[2], 195.9977, abs_tol=0.001)

        # test with a list as arg
        in_list = [48, 52, 55]
        out_list = midiToHz(in_list)
        assert type(out_list) is list
        assert math.isclose(out_list[0], 130.8128, abs_tol=0.001)
        assert math.isclose(out_list[1], 164.8138, abs_tol=0.001)
        assert math.isclose(out_list[2], 195.9977, abs_tol=0.001)

    def test_midiToTranspo(self):
        # test with a number as arg
        in_num = 60
        out_num = midiToTranspo(in_num)
        assert isinstance(out_num, (int, float))
        assert out_num == 1.0

        # test with a tuple as arg
        in_tuple = (48, 60, 72)
        out_tuple = midiToTranspo(in_tuple)
        assert type(out_tuple) is tuple
        assert math.isclose(out_tuple[0], 0.5, abs_tol=0.00001)
        assert out_tuple[1] == 1.0
        assert math.isclose(out_tuple[2], 2.0, abs_tol=0.00001)

        # test with a list as arg
        in_list = [48, 60, 72]
        out_list = midiToTranspo(in_list)
        assert type(out_list) is list
        assert math.isclose(out_list[0], 0.5, abs_tol=0.00001)
        assert out_list[1] == 1.0
        assert math.isclose(out_list[2], 2.0, abs_tol=0.00001)

    def test_sampsToSec(self):
        # test with a number as arg
        in_num = 48000
        out_num = sampsToSec(in_num)
        assert isinstance(out_num, (int, float))
        assert out_num == 1.0

        # test with a tuple as arg
        in_tuple = (24000, 48000, 96000)
        out_tuple = sampsToSec(in_tuple)
        assert type(out_tuple) is tuple
        assert out_tuple[0] == 0.5
        assert out_tuple[1] == 1.0
        assert out_tuple[2] == 2.0

        # test with a list as arg
        in_list = [24000, 48000, 96000]
        out_list = sampsToSec(in_list)
        assert type(out_list) is list
        assert out_list[0] == 0.5
        assert out_list[1] == 1.0
        assert out_list[2] == 2.0

    def test_secToSamps(self):
        # test with a number as arg
        in_num = 1.0
        out_num = secToSamps(in_num)
        assert isinstance(out_num, (int, float))
        assert out_num == 48000

        # test with a tuple as arg
        in_tuple = (0.5, 1.0, 2.0)
        out_tuple = secToSamps(in_tuple)
        assert type(out_tuple) is tuple
        assert out_tuple[0] == 24000
        assert out_tuple[1] == 48000
        assert out_tuple[2] == 96000

        # test with a list as arg
        in_list = [0.5, 1.0, 2.0]
        out_list = secToSamps(in_list)
        assert type(out_list) is list
        assert out_list[0] == 24000
        assert out_list[1] == 48000
        assert out_list[2] == 96000

    def test_beatToDur(self):
        # test with a number as arg
        bpm = 60
        in_num = 1
        out_num = beatToDur(in_num, bpm)
        assert isinstance(out_num, (int, float))
        assert out_num == 1.0

        # test with a tuple as arg
        in_tuple = (1 / 2, 1, 2)
        out_tuple = beatToDur(in_tuple, bpm)
        assert type(out_tuple) is tuple
        assert out_tuple[0] == 0.5
        assert out_tuple[1] == 1.0
        assert out_tuple[2] == 2.0

        # test with a list as arg
        in_list = [1 / 2, 1, 2]
        out_list = beatToDur(in_list, bpm)
        assert type(out_list) is list
        assert out_list[0] == 0.5
        assert out_list[1] == 1.0
        assert out_list[2] == 2.0

    def test_linToCosCurve(self):
        n = 512
        l = [(0,0), (0.5, 1), (1,0)]
        pts = linToCosCurve(l, yrange=[0, 1], totaldur=1, points=n)
        for i in range(512):
            v = -0.5 * math.cos(2 * math.pi * i / n) + 0.5
            assert math.isclose(pts[i][1], v, abs_tol=0.000001)

        log10ymin = math.log10(0.001)
        log10ymax = math.log10(1.0)
        l = [(0,0.001), (0.5, 1), (1,0.001)]
        pts = linToCosCurve(l, yrange=[0.001, 1], totaldur=1, points=n, log=True)
        for i in range(512):
            v = pow(10, (-0.5 * math.cos(2 * math.pi * i / n) + 0.5) * (log10ymax - log10ymin) + log10ymin)
            assert math.isclose(pts[i][1], v, abs_tol=0.000001)

    def test_rescale(self):
        # test with a number as arg
        in_num = 0.0
        out_num = rescale(in_num, xmin=-1.0, xmax=1.0, ymin=0.0, ymax=1.0, xlog=False, ylog=False)
        assert isinstance(out_num, (int, float))
        assert out_num == 0.5

        log10ymin = math.log10(0.001)
        log10ymax = math.log10(1.0)
        out_num = rescale(in_num, xmin=-1.0, xmax=1.0, ymin=0.001, ymax=1.0, xlog=False, ylog=True)
        assert math.isclose(out_num, pow(10, 0.5 * (log10ymax - log10ymin) + log10ymin), abs_tol=0.000001)

        # How to test xlog=True ?

        # test with a list as arg

#    def test_floatmap(self):
#        pass

#    def test_distanceToSegment(self):
#        pass

#    def test_reducePoints(self):
#        pass

class TestServerQueriesFunctions:

    def test_serverCreated(self):
        assert serverCreated() == False
        s = Server()
        assert serverCreated() == True

    def test_serverBooted(self):
        s = Server()
        assert serverBooted() == False
        s.boot()
        assert serverBooted() == True
        s.shutdown()
