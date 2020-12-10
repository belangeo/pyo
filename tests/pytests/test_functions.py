import os
import math
import pytest
from pyo import *

class TestInitFunctions:

    def test_getPyoKeywords(self):
        kwds = getPyoKeywords()
        assert type(kwds) is list
        assert "PyoObject" in kwds
        assert "SineLoop" in kwds
        assert "Particle" in kwds
        assert len(kwds) == 368

    def test_getPyoExamples(self):
        examples = getPyoExamples(fullpath=True)
        for cat in examples:
            for f in examples[cat]:
                assert os.path.isfile(f)

        root = os.path.split(os.path.split(examples['01-intro'][0])[0])[0]

        examples = getPyoExamples()
        for cat in examples:
            for f in examples[cat]:
                assert os.path.isfile(os.path.join(root, cat, f))

@pytest.mark.usefixtures("audio_server")
class TestSoundfileFunctions:

    def test_sndinfo(self):
        path = os.path.join(SNDS_PATH, "transparent.aif")
        info = sndinfo(path)
        assert info[0] == 29877  # number of samples
        assert round(info[1], 5) == 0.67748  # duration in sec
        assert info[2] == 44100.0  # sampling rate
        assert info[3] == 1  # number of channels
        assert info[4] == 'AIFF'  # file format
        assert info[5] == '16 bit int'  # sample type

    def test_savefile(self):
        path = os.path.join(os.path.expanduser("~"), "temporary_soundfile.wav")
        sr = 44100
        dur = 1
        samples = [math.sin(math.pi * 2 * i / sr * dur) for i in range(sr)]
        savefile(samples, path, sr=sr, channels=1, fileformat=0, sampletype=3)

        # check saved file header values.
        info = sndinfo(path)
        assert info[0] == 44100  # number of samples
        assert round(info[1], 5) == 1.0  # duration in sec
        assert info[2] == 44100.0  # sampling rate
        assert info[3] == 1  # number of channels
        assert info[4] == 'WAVE'  # file format
        assert info[5] == '32 bit float'  # sample type

        # reload the sound and compare with the original waveform.
        table = SndTable(path)
        assert sr == table.getSize()
        sum = 0
        for i in range(sr):
            sum += samples[i] - table.get(i)
        assert math.isclose(sum, 0.0, abs_tol=0.000000001)

        os.remove(path)

    def test_savefileFromTable(self):
        path = os.path.join(SNDS_PATH, "transparent.aif")
        info = sndinfo(path)
        ori = SndTable(path)

        outpath = os.path.join(os.path.expanduser("~"), "temporary_soundfile.wav")
        savefileFromTable(ori, outpath, fileformat=0, sampletype=3)

        # check saved file header values.
        info2 = sndinfo(outpath)
        assert info[0] == info2[0]  # number of samples
        assert info[1] == info2[1]  # duration in sec
        assert info[2] == info2[2]  # sampling rate
        assert info[3] == info2[3]  # number of channels

        # reload the sound and compare with the original waveform.
        table = SndTable(outpath)
        sum = 0
        for i in range(table.getSize()):
            sum += ori.get(i) - table.get(i)
        assert math.isclose(sum, 0.0, abs_tol=0.000000001)

        os.remove(outpath)

@pytest.mark.usefixtures("audio_server")
class TestResamplingFunctions:

    def test_upsamp(self):
        home = os.path.expanduser('~')
        path = os.path.join(SNDS_PATH, "transparent.aif")
        nsamps = sndinfo(path)[0]
        sr = sndinfo(path)[2]

        # upsample a signal 3 times
        upfile = os.path.join(home, 'trans_upsamp_3.aif')
        upsamp(path, upfile, 3, 256)
        info = sndinfo(upfile)
        assert info[2] == (sr * 3)
        assert info[0] == (nsamps * 3)

        os.remove(upfile)

    def test_downsamp(self):
        home = os.path.expanduser('~')
        path = os.path.join(SNDS_PATH, "transparent.aif")
        nsamps = sndinfo(path)[0]
        sr = sndinfo(path)[2]

        # downsample the upsampled signal 4 times
        downfile = os.path.join(home, 'trans_downsamp_4.aif')
        downsamp(path, downfile, 4, 256)
        info = sndinfo(downfile)
        assert info[2] == (sr / 4)
        assert math.isclose(info[0], nsamps / 4, abs_tol=1.0)

        os.remove(downfile)

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

        log10ymin = math.log10(0.001);
        log10ymax = math.log10(1.0);
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

        log10ymin = math.log10(0.001);
        log10ymax = math.log10(1.0);
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
