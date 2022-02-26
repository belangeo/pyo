import os
import pytest
import mock
import numpy
from utilities import *
from pyo import *


class TestPyoBaseObject_ServerNotCreated:

    def test_PyoObjectBase_server_not_created_exception(self):
        with pytest.raises(PyoServerStateException):
            a = Sine()


@pytest.mark.usefixtures("audio_server")
class TestPyoBaseObject:

    def test_PyoObjectBase_server_not_booted_exception(self, audio_server):
        audio_server.shutdown()
        with pytest.raises(PyoServerStateException):
            a = Sine()

    def test_PyoObjectBase(self, audio_server):
        a = Sine(freq=[100, 110, 120, 130])
        # getters
        assert a.getSamplingRate() == 48000
        assert a.getBufferSize() == 512
        assert a.getServer() == audio_server._server
        # number of streams managed by the object
        assert len(a.getBaseObjects()) == 4
        assert len(a) == 4
        # subscripts
        for i in range(len(a)):
            assert a[i] == a.getBaseObjects()[i]
        # can be used as an iterator
        for i, obj in enumerate(a):
            assert obj == a.getBaseObjects()[i]
        # dir() returns the list of attributes
        assert sorted(dir(a)) == sorted(['freq', 'phase', 'mul', 'add'])
        # dump() returns info about the object's state
        assert "Number of audio streams: 4" in a.dump()
        assert "freq: [100, 110, 120, 130]" in a.dump()

        # autostart utilities
        a.setStopDelay(5.3)
        assert a.getStopDelay() == 5.3

        assert a._allow_auto_start
        a.allowAutoStart(False)
        assert not a._allow_auto_start

        assert not a._use_wait_time_on_stop
        a.useWaitTimeOnStop()
        assert a._use_wait_time_on_stop

        tab = NewTable(length=2, chnls=1)
        rec = TableRec(Sine(500), tab, .01)
        amp = TrigVal(rec["trig"], 0.5)
        osc = Osc(tab, tab.getRate(), mul=Fader(1, 1, mul=.2))
        # "osc" can't know for sure that "rec" should be linked
        # to this dsp chain, so we add it manually.
        assert osc._linked_objects == []
        osc.addLinkedObject(rec)
        assert osc._linked_objects == [rec]

    def test_indexing_slice(self, capsys):
        a = Sine([1, 2, 3, 4, 5, 6, 7, 8])
        b = a[2:6]
        assert len(b) == 4

    def test_indexing_wrong_accessor(self, capsys):
        a = Sine()
        b = a["wrong"]
        captured = capsys.readouterr()
        assert captured.out == "Object Sine has no stream named 'wrong'!\n"
        assert b is None

    def test_indexing_too_large(self, capsys):
        a = Sine([1, 2])
        b = a[2]
        captured = capsys.readouterr()
        assert captured.out == "'i' too large in indexing audio object Sine!\n"
        assert b is None

    def test_repr(self):
        a = Sine()
        assert str(a) == "< Instance of Sine class >"

@pytest.mark.usefixtures("audio_server")
class TestPyoObject:

    def test_isAudioObject(self):
        a = Sine()
        assert isAudioObject(a)
        t = NewTable(1)
        assert not isAudioObject(t)

    def test_aritmetic_with_number(self, audio_server):
        a = Sig(1)
        a.mul = 1
        a.add = 0

        m_add = a + 1
        assert m_add.get() == 2
        m_sub = a - 1
        assert m_sub.get() == 0
        m_mul = a * 2
        assert m_mul.get() == 2
        m_div = a / 2
        assert m_div.get() == 0.5

        # in-place
        i_add = Sig(1)
        i_add += 1
        i_sub = Sig(1)
        i_sub -= 1
        i_mul = Sig(1)
        i_mul *= 2
        i_div = Sig(1)
        i_div /= 2

        with StartAndAdvanceOneBuf(audio_server):
            assert i_add.get() == 2
            assert i_sub.get() == 0
            assert i_mul.get() == 2
            assert i_div.get() == 0.5

    def test_aritmetic_with_number_list(self, audio_server):
        a = Sig(1)
        b = [0.5, 2]

        m_add = a + b
        assert m_add.get(all=True) == [1.5, 3]
        m_sub = a - b
        assert m_sub.get(all=True) == [0.5, -1]
        m_mul = a * b
        assert m_mul.get(all=True) == [0.5, 2]
        m_div = a / b
        assert m_div.get(all=True) == [2, 0.5]

        # in-place
        i_add = Sig(1)
        i_add += b
        i_sub = Sig(1)
        i_sub -= b
        i_mul = Sig(1)
        i_mul *= b
        i_div = Sig(1)
        i_div /= b

        with StartAndAdvanceOneBuf(audio_server):
            # in-place aritmetic doesn't change the number of streams managed by the object.
            assert i_add.get() == 1.5
            assert i_sub.get() == 0.5
            assert i_mul.get() == 0.5
            assert i_div.get() == 2

    def test_reverse_aritmetic_with_number(self, audio_server):
        a = Sig(1)

        m_add = 1 + a
        assert m_add.get() == 2
        m_sub = 1 - a
        assert m_sub.get() == 0
        m_mul = 2 * a
        assert m_mul.get() == 2
        m_div = 2 / a
        assert m_div.get() == 2

    def test_reverse_aritmetic_with_number_list(self, audio_server):
        a = Sig(2)
        b = [1, 2]

        m_add = b + a
        assert m_add.get(all=True) == [3, 4]
        m_sub = b - a
        assert m_sub.get(all=True) == [-1, 0]
        m_mul = b * a
        assert m_mul.get(all=True) == [2, 4]
        m_div = b / a
        assert m_div.get(all=True) == [0.5, 1]

    def test_aritmetic_with_audio_object(self, audio_server):
        a = Sig(1)
        b1 = Sig(1)
        b2 = Sig(2)

        m_add = a + b1
        assert m_add.get() == 2
        m_sub = a - b1
        assert m_sub.get() == 0
        m_mul = a * b2
        assert m_mul.get() == 2
        m_div = a / b2
        assert m_div.get() == 0.5

        # in-place
        i_add = Sig(1)
        i_add += b1
        i_sub = Sig(1)
        i_sub -= b1
        i_mul = Sig(1)
        i_mul *= b2
        i_div = Sig(1)
        i_div /= b2

        with StartAndAdvanceOneBuf(audio_server):
            assert i_add.get() == 2
            assert i_sub.get() == 0
            assert i_mul.get() == 2
            assert i_div.get() == 0.5
    
    def test_aritmetic_with_audio_object_list(self, audio_server):
        a = Sig(1)
        b = Sig([0.5, 2])

        m_add = a + b
        assert m_add.get(all=True) == [1.5, 3]
        m_sub = a - b
        assert m_sub.get(all=True) == [0.5, -1]
        m_mul = a * b
        assert m_mul.get(all=True) == [0.5, 2]
        m_div = a / b
        assert m_div.get(all=True) == [2, 0.5]

        # in-place
        i_add = Sig(1)
        i_add += b
        i_sub = Sig(1)
        i_sub -= b
        i_mul = Sig(1)
        i_mul *= b
        i_div = Sig(1)
        i_div /= b

        with StartAndAdvanceOneBuf(audio_server):
            # in-place aritmetic doesn't change the number of streams managed by the object.
            assert i_add.get() == 1.5
            assert i_sub.get() == 0.5
            assert i_mul.get() == 0.5
            assert i_div.get() == 2

    def test_other_aritmetic_with_number(self, audio_server):
        a = Sig(2)

        m_exp = a ** 2
        m_rexp = 2 ** a
        m_mod = a % 2
        m_neg = -a

        with StartAndAdvanceOneBuf(audio_server):
            assert m_exp.get() == 4
            assert m_rexp.get() == 4
            assert m_mod.get() == 0
            assert m_neg.get() == -2

    def test_other_aritmetic_with_audio_object(self, audio_server):
        a = Sig(2)
        b = Sig(2)

        m_exp = a ** b
        m_mod = a % b

        with StartAndAdvanceOneBuf(audio_server):
            assert m_exp.get() == 4
            assert m_mod.get() == 0

    def test_comparison_with_number(self, audio_server):
        a = Sig(1)
        b = 2

        m_lt = a < b
        m_lte = a <= b
        m_gt = a > b
        m_gte = a >= b
        m_eq = a == b
        m_ne = a != b

        with StartAndAdvanceOneBuf(audio_server):
            assert m_lt.get() == 1
            assert m_lte.get() == 1
            assert m_gt.get() == 0
            assert m_gte.get() == 0
            assert m_eq.get() == 0
            assert m_ne.get() == 1

    def test_comparison_with_audio_object(self, audio_server):
        a = Sig(1)
        b = Sig(2)

        m_lt = a < b
        m_lte = a <= b
        m_gt = a > b
        m_gte = a >= b
        m_eq = a == b
        m_ne = a != b

        with StartAndAdvanceOneBuf(audio_server):
            assert m_lt.get() == 1
            assert m_lte.get() == 1
            assert m_gt.get() == 0
            assert m_gte.get() == 0
            assert m_eq.get() == 0
            assert m_ne.get() == 1

    def test_comparison_with_None(self, audio_server):
        a = Sig(1)
        b = None

        m_lt = a < b
        m_lte = a <= b
        m_gt = a > b
        m_gte = a >= b
        m_eq = a == b
        m_ne = a != b

        assert m_lt == False
        assert m_lte == False
        assert m_gt == False
        assert m_gte == False
        assert m_eq == False
        assert m_ne == True

    def test_isPlaying(self):
        a = Sine([100, 200])
        assert a.isPlaying() == True
        assert a.isPlaying(all=True) == [True, True]
        a.stop()
        assert a.isPlaying() == False
        assert a.isPlaying(all=True) == [False, False]

    def test_isOutputting(self):
        a = Sine([100, 200])
        assert a.isOutputting() == False
        assert a.isOutputting(all=True) == [False, False]
        a.out()
        assert a.isOutputting() == True
        assert a.isOutputting(all=True) == [True, True]

    def test_get(self, audio_server):
        a = Sig([100, 200])
        assert a.get() == 100
        assert a.get(all=True) == [100, 200]

        a.value = [25, 50]

        with StartAndAdvanceOneBuf(audio_server):
            assert a.get() == 25
            assert a.get(all=True) == [25, 50]

    def test_play(self, audio_server):
        a = Sine()
        assert a.isPlaying() == True

        a.stop()

        with Start(audio_server) as cxt:
            # wait 0.5 second before playing and play for 0.5 second
            a.play(dur=0.5, delay=0.5)
            cxt.advance(0.25)
            assert a.isPlaying() == False
            cxt.advance(0.5)
            assert a.isPlaying() == True
            cxt.advance(0.5)
            assert a.isPlaying() == False

    def test_out(self, audio_server):
        a = Sine(mul=0).stop()

        with Start(audio_server) as cxt:
            # wait 0.5 second before playing and play for 0.5 second
            a.out(dur=0.5, delay=0.5)
            cxt.advance(0.25)
            assert a.isPlaying() == False
            cxt.advance(0.5)
            assert a.isPlaying() == True
            cxt.advance(0.5)
            assert a.isPlaying() == False

        num_chnls = 4

        # reinit the audio server with 8 channels
        audio_server.shutdown()
        audio_server.reinit(sr=48000, buffersize=512, nchnls=8)
        audio_server.boot()

        a = Sine([(i + 1) * 100 for i in range(num_chnls)], mul=0.3)
        assert a.isOutputting() == False

        a.out(chnl=0, inc=1, dur=0, delay=0)
        assert a.isOutputting() == True

        # normal order => [0, 1, 2, 3]
        for i in range(num_chnls):
            assert a[i]._getStream().getOutputChannel() == i

        # start at 0, increment by 2 => [0, 2, 4, 6]
        a.out(chnl=0, inc=2, dur=0, delay=0)
        for i in range(num_chnls):
            assert a[i]._getStream().getOutputChannel() == i * 2

        # start at 1, increment by 1 => [1, 2, 3, 4]
        a.out(chnl=1, inc=1, dur=0, delay=0)
        for i in range(num_chnls):
            assert a[i]._getStream().getOutputChannel() == i + 1

        # specific order => [3, 6, 5, 1]
        out_chnls = [3, 6, 5, 1]
        a.out(chnl=out_chnls, inc=1, dur=0, delay=0)
        for i in range(num_chnls):
            assert a[i]._getStream().getOutputChannel() == out_chnls[i]

        # scrambled order
        a.out(chnl=-1, inc=1, dur=0, delay=0)
        assert [a[i]._getStream().getOutputChannel() for i in range(num_chnls)] != [0, 1, 2, 3]

    def test_stop(self, audio_server):
        a = Sine()
        assert a.isPlaying() == True

        a.stop()
        assert a.isPlaying() == False

        a.play()

        with Start(audio_server) as cxt:
            a.stop(wait=0.25)
            assert a.isPlaying() == True
            cxt.advance(0.5)
            assert a.isPlaying() == False

    def test_mix(self):
        a = Sine([1,2,3,4,5,6,7,8])
        assert len(a) == 8
        b = a.mix(2)
        assert len(b) == 2

        a = Sine()
        assert len(a) == 1
        b = a.mix(8)
        assert len(b) == 8

        a = Sine([1,2])
        assert len(a) == 2
        b = a.mix(5)
        assert len(b) == 5
        c = a.mix(3)
        assert len(c) == 3

    def test_mix_2(self, audio_server):
        a = Sig([1,1])
        b = a.mix()
        assert len(b) == 1

        c = Sig([1.5, 0.5, 0.25, 0.75])
        d = c.mix(2)
        assert len(d) == 2

        with StartAndAdvanceOneBuf(audio_server):
            assert b.get() == 2
            assert d.get(all=True) == [1.75, 1.25]

    def test_range(self):
        a = Sine().range(0, 1)
        assert a.mul == 0.5
        assert a.add == 0.5
        b = Sine().range(-100, 200)
        assert b.mul == 150
        assert b.add == 50

        c = Sine([1, 2]).range([100, 500], [200, 1000])
        assert c.mul == [50, 250]
        assert c.add == [150, 750]

    def test_set(self, audio_server):
        func = mock.Mock()

        a = Sine()
        self.set_b = Sine(mul=[1,1,1,1])
        a.set(attr="freq", value=500, port=0.3, callback=None)
        self.set_b.set(attr="mul", value=[0, 0.25, 0.5, 0.75], port=0.3, callback=func)

        with StartAndAdvance(audio_server, adv_time=0.5):
            assert a.freq == 500
            assert self.set_b.mul == [0, 0.25, 0.5, 0.75]
            func.assert_called()

    def test_set_interupt(self, audio_server):
        func = mock.Mock()

        self.set_b = Sine(mul=[1,1,1,1])
        self.set_b.set(attr="mul", value=[0, 0.25, 0.5, 0.75], port=0.5, callback=func)

        with StartAndAdvance(audio_server, adv_time=0.25) as cxt:
            self.set_b.set(attr="mul", value=[0.125, 0.5, 0, 0.25], port=0.3, callback=func)
            cxt.advance(0.5)
            assert self.set_b.mul == [0.125, 0.5, 0, 0.25]
            func.assert_called_once()

    def test_number_of_streams(self, audio_server):
        a = Sine()
        b = Sine(mul=a)
        del b
        del a
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_2(self, audio_server):
        a = Sine()
        b = Sine()
        b.setMul(a)
        del b
        del a
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_3(self, audio_server):
        a = Sine()
        b = Sine()
        b.setFreq(a)
        del b
        del a
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_4(self, audio_server):
        a = Sine()
        b = Sine()
        c = Sine()
        c.setFreq(a)
        c.setFreq(b)
        del a, b, c
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_5(self, audio_server):
        a = Sig(0)
        b = Sig(0)
        c = Sig(0)
        c.setMul(a*b)
        del a, b, c
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_6(self, audio_server):
        a = Sig(0)
        b = Sig(0)
        c = Sig(0)
        d = Sig(0)
        d.setMul(a+b+c)
        del a, b, c, d
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_sub_streams(self, audio_server):
        t = CosTable([(0,0), (100,1), (500,.3), (8191,0)])
        beat = Beat(time=.125, taps=16, w1=[90,80], w2=50, w3=35, poly=1).play()
        trmid = TrigXnoiseMidi(beat, dist=12, mrange=(60, 96))
        trhz = Snap(trmid, choice=[0,2,3,5,7,8,10], scale=1)
        tr2 = TrigEnv(beat, table=t, dur=beat['dur'], mul=beat['amp'])
        tf = TrigFunc(beat["end"], lambda : True)
        a = Sine(freq=trhz, mul=tr2*0.3)
        del a, tf, tr2, trhz, trmid, beat, t
        assert audio_server.getNumberOfStreams() == 0

    def test_number_of_streams_pv_process(self, audio_server):
        size = 1024
        olaps = 4
        num = olaps * 2  # number of streams for ffts

        src = Sine(freq=[100,110], mul=0.15)
        delsrc = Delay(src, delay=size / audio_server.getSamplingRate() * 2).out()

        # duplicates bin regions and delays to match the number of channels * overlaps
        def duplicate(li, num):
            tmp = [x for x in li for i in range(num)]
            return tmp

        binmin = duplicate([3, 10, 20, 27, 55, 100], num)
        binmax = duplicate([5, 15, 30, 40, 80, 145], num)
        delays = duplicate([80, 20, 40, 100, 60, 120], num)
        # delays conversion : number of frames -> seconds
        for i in range(len(delays)):
            delays[i] = delays[i] * (size // 2) / audio_server.getSamplingRate()

        fin = FFT(src * 1.25, size=size, overlaps=olaps)

        # splits regions between `binmins` and `binmaxs` with time variation
        lfo = Sine(0.1, mul=0.65, add=1.35)
        bins = Between(fin["bin"], min=binmin, max=binmax * lfo)
        swre = fin["real"] * bins
        swim = fin["imag"] * bins
        # apply delays with mix to match `num` audio streams
        delre = Delay(swre, delay=delays, feedback=0.7, maxdelay=2).mix(num)
        delim = Delay(swim, delay=delays, feedback=0.7, maxdelay=2).mix(num)

        fout = IFFT(delre, delim, size=size, overlaps=olaps).mix(2).out()

        del fout, delim, delre, swre, swim, bins, lfo, fin, delsrc, src

        assert audio_server.getNumberOfStreams() == 0

@pytest.mark.usefixtures("audio_server")
class TestPyoTableObject:

    def test_isTableObject(self):
        a = Sine()
        assert not isTableObject(a)
        t = NewTable(1)
        assert isTableObject(t)

    def test_write(self):
        path = os.path.join(os.path.expanduser("~"), "temporary_file.txt")

        for oneline in [True, False]:
            t = HarmTable()
            t.write(path, oneline=oneline)

            with open(path, "r") as f:
                values = eval(f.read())

            assert len(values[0]) == t.getSize()
            for i in range(len(values)):
                assert math.isclose(float(values[0][i]), t.get(i), abs_tol=0.0000001)

        os.remove(path)

    def test_read(self):
        path = os.path.join(os.path.expanduser("~"), "temporary_file.txt")

        num = 64

        data = [[random.random() for i in range(num)]]
        with open(path, "w") as f:
            f.write(str(data))

        t = DataTable(size=1)
        t.read(path)

        assert t.getSize() == num

        for i in range(num):
            assert math.isclose(t.get(i), data[0][i], abs_tol=0.0000001)

        os.remove(path)

    def test_read_2(self):
        path = os.path.join(os.path.expanduser("~"), "temporary_file.txt")

        chnls = 2
        num = 64

        data = [[random.random() for i in range(num)] for j in range(chnls)]
        with open(path, "w") as f:
            f.write(str(data))

        t = DataTable(size=1, chnls=chnls)
        t.read(path)

        assert len(t) == chnls
        assert t.getSize(all=False) == num
        assert t.getSize(all=True) == [num for i in range(chnls)]

        for j in range(chnls):
            for i in range(num):
                print(t.get(i), data[0][j])
                assert math.isclose(t.get(i)[j], data[j][i], abs_tol=0.0000001)

        os.remove(path)

    def test_getBuffer(self):
        t = DataTable(size=10, chnls=1, init=list(range(10)))

        arr = numpy.asarray(t.getBuffer())
        arr[:] = arr[::-1] # reverse the array in-place

        # compare the table content with a reversed list
        for i, v in enumerate(range(9, -1, -1)):
            assert t.get(i) == v

        # test chnl number out-of-bounds
        buf = t.getBuffer(chnl=1)
        assert buf is None

    def test_setSize(self):
        t = LinTable([(0, 0), (511, 1)], size=512)
        assert t.getSize() == 512
        t.setSize(5)
        assert t.getSize() == 5

    def test_getSize(self):
        t = SndTable(os.path.join(os.getcwd(), "pouf2.wav"))
        assert t.getSize(all=False) == 11036
        assert t.getSize(all=True) == [11036, 11036]

    def test_put(self):
        t = DataTable(size=10)
        assert t.get(0) == 0
        t.put(value=1.0, pos=0)
        assert t.get(0) == 1.0
        t.put(value=1.0, pos=5)
        assert t.get(5) == 1.0
        t.put(value=1.0, pos=-5)
        assert t.get(5) == 1.0

    def test_get(self):
        t = DataTable(size=4, chnls=1, init=[1, 2, 3, 4])
        assert t.get(0) == 1
        assert t.get(-2) == 3
        t = DataTable(size=4, chnls=2, init=[[1, 2, 3, 4], [5, 6, 7, 8]])
        assert t.get(0) == [1, 5]

    def test_getTable(self):
        t = DataTable(size=4, chnls=2, init=[[1, 2, 3, 4], [5, 6, 7, 8]])
        assert t.getTable(all=False) == [1, 2, 3, 4]
        assert t.getTable(all=True) == [[1, 2, 3, 4], [5, 6, 7, 8]]

    def test_normalize(self):
        t = DataTable(size=4, chnls=1, init=[-0.5, -0.25, 0.25, 0.5])
        t.normalize(level=1)
        assert t.getTable() == [-1, -0.5, 0.5, 1]
        t.normalize(level=0.5)
        assert t.getTable() == [-0.5, -0.25, 0.25, 0.5]

    def test_reset(self):
        t = DataTable(size=4, chnls=1, init=[-0.5, -0.25, 0.25, 0.5])
        t.reset()
        for i in range(4):
            assert t.get(i) == 0

    # TO BE IMPLEMENTED
    #def test_removeDC(self):

    def test_reverse(self):
        t = DataTable(size=10, chnls=1, init=list(range(10)))
        t.reverse()
        assert t.getTable() == list(range(9, -1, -1))

    def test_invert(self):
        t = DataTable(size=4, chnls=1, init=[-1, -0.5, -0.25, 0.25])
        t.invert()
        assert t.getTable() == [1, 0.5, 0.25, -0.25]

    def test_rectify(self):
        t = DataTable(size=4, chnls=1, init=[-1, -0.5, -0.25, 0.25])
        t.rectify()
        assert t.getTable() == [1, 0.5, 0.25, 0.25]

    def test_pow(self):
        t = DataTable(size=4, chnls=1, init=[-2, -1, 1, 2])
        t.pow()
        assert t.getTable() == [-1024, -1, 1, 1024]

        t = DataTable(size=4, chnls=1, init=[-2, -1, 1, 2])
        t.pow(2)
        assert t.getTable() == [-4, -1, 1, 4]

    def test_bipolarGain(self):
        t = DataTable(size=4, chnls=1, init=[-1, -0.5, 0.5, 1])
        t.bipolarGain(gpos=1, gneg=0.5)
        assert t.getTable() == [-0.5, -0.25, 0.5, 1]

        t = DataTable(size=4, chnls=1, init=[-1, -0.5, 0.5, 1])
        t.bipolarGain(gpos=0.5, gneg=1)
        assert t.getTable() == [-1, -0.5, 0.25, 0.5]

        t = DataTable(size=4, chnls=1, init=[-1, -0.5, 0.5, 1])
        t.bipolarGain(gpos=0.5, gneg=0.5)
        assert t.getTable() == [-0.5, -0.25, 0.25, 0.5]

    # TO BE IMPLEMENTED
    #def test_lowpass(self):

    def test_fadein(self, audio_server):
        dur = 4 / audio_server.getSamplingRate()

        # linear
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadein(dur=dur)
        assert t.getTable() == [0, 0.25, 0.5, 0.75, 1]
        # sqrt
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadein(dur=dur, shape=1)
        assert t.get(0) == 0
        assert math.isclose(t.get(1), math.sqrt(0.25), abs_tol=0.0000001)
        assert math.isclose(t.get(2), math.sqrt(0.5), abs_tol=0.0000001)
        assert math.isclose(t.get(3), math.sqrt(0.75), abs_tol=0.0000001)
        assert t.get(4) == 1.0
        # sin
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadein(dur=dur, shape=2)
        assert t.get(0) == 0
        assert math.isclose(t.get(1), math.sin(0.25 * math.pi / 2), abs_tol=0.0000001)
        assert math.isclose(t.get(2), math.sin(0.5 * math.pi / 2), abs_tol=0.0000001)
        assert math.isclose(t.get(3), math.sin(0.75 * math.pi / 2), abs_tol=0.0000001)
        assert t.get(4) == 1.0
        # squared
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadein(dur=dur, shape=3)
        assert t.get(0) == 0
        assert math.isclose(t.get(1), 0.25 * 0.25, abs_tol=0.0000001)
        assert math.isclose(t.get(2), 0.5 * 0.5, abs_tol=0.0000001)
        assert math.isclose(t.get(3), 0.75 * 0.75, abs_tol=0.0000001)
        assert t.get(4) == 1.0
        # wronf shape value (defaults to linear)
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadein(dur=dur, shape=4)
        assert t.getTable() == [0, 0.25, 0.5, 0.75, 1]

    def test_fadeout(self, audio_server):
        dur = 4 / audio_server.getSamplingRate()

        # linear
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadeout(dur=dur)
        assert t.getTable() == [1, 0.75, 0.5, 0.25, 0]
        # sqrt
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadeout(dur=dur, shape=1)
        assert t.get(0) == 1
        assert math.isclose(t.get(1), math.sqrt(0.75), abs_tol=0.0000001)
        assert math.isclose(t.get(2), math.sqrt(0.5), abs_tol=0.0000001)
        assert math.isclose(t.get(3), math.sqrt(0.25), abs_tol=0.0000001)
        assert t.get(4) == 0
        # sin
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadeout(dur=dur, shape=2)
        assert t.get(0) == 1
        assert math.isclose(t.get(1), math.sin(0.75 * math.pi / 2), abs_tol=0.0000001)
        assert math.isclose(t.get(2), math.sin(0.5 * math.pi / 2), abs_tol=0.0000001)
        assert math.isclose(t.get(3), math.sin(0.25 * math.pi / 2), abs_tol=0.0000001)
        assert t.get(4) == 0
        # squared
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadeout(dur=dur, shape=3)
        assert t.get(0) == 1
        assert math.isclose(t.get(1), 0.75 * 0.75, abs_tol=0.0000001)
        assert math.isclose(t.get(2), 0.5 * 0.5, abs_tol=0.0000001)
        assert math.isclose(t.get(3), 0.25 * 0.25, abs_tol=0.0000001)
        assert t.get(4) == 0
        # wrong shape value (defaults to linear)
        t = DataTable(size=5, chnls=1, init=[1, 1, 1, 1, 1])
        t.fadeout(dur=dur, shape=4)
        assert t.getTable() == [1, 0.75, 0.5, 0.25, 0]

    def test_add(self):
        # float
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.add(0.5)
        assert t.getTable() == [1.5, 1.5, 1.5, 1.5]
        # list
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.add([0.25, 0.5, 0.75, 1])
        assert t.getTable() == [1.25, 1.5, 1.75, 2]
        # table
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t2 = DataTable(size=4, chnls=1, init=[0.25, 0.5, 0.75, 1])
        t.add(t2)
        assert t.getTable() == [1.25, 1.5, 1.75, 2]
        # list of list
        t = DataTable(size=4, chnls=2, init=[[1, 1, 1, 1], [1, 1, 1, 1]])
        t.add([[0.25, 0.5, 0.75, 1], [1, 0.75, 0.5, 0.25]])
        assert t.getTable(all=True) == [[1.25, 1.5, 1.75, 2], [2, 1.75, 1.5, 1.25]]

    def test_mul(self):
        # float
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.mul(2)
        assert t.getTable() == [2, 2, 2, 2]
        # list
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.mul([0.25, 0.5, 0.75, 2])
        assert t.getTable() == [0.25, 0.5, 0.75, 2]
        # table
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t2 = DataTable(size=4, chnls=1, init=[0.25, 0.5, 0.75, 2])
        t.mul(t2)
        assert t.getTable() == [0.25, 0.5, 0.75, 2]
        # list of list
        t = DataTable(size=4, chnls=2, init=[[1, 1, 1, 1], [1, 1, 1, 1]])
        t.mul([[0.25, 0.5, 0.75, 2], [2, 0.75, 0.5, 0.25]])
        assert t.getTable(all=True) == [[0.25, 0.5, 0.75, 2], [2, 0.75, 0.5, 0.25]]

    def test_sub(self):
        # float
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.sub(0.5)
        assert t.getTable() == [0.5, 0.5, 0.5, 0.5]
        # list
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.sub([0.25, 0.5, 0.75, 1])
        assert t.getTable() == [0.75, 0.5, 0.25, 0]
        # table
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t2 = DataTable(size=4, chnls=1, init=[0.25, 0.5, 0.75, 1])
        t.sub(t2)
        assert t.getTable() == [0.75, 0.5, 0.25, 0]
        # list of list
        t = DataTable(size=4, chnls=2, init=[[1, 1, 1, 1], [1, 1, 1, 1]])
        t.sub([[0.25, 0.5, 0.75, 1], [1, 0.75, 0.5, 0.25]])
        assert t.getTable(all=True) == [[0.75, 0.5, 0.25, 0], [0, 0.25, 0.5, 0.75]]

    def test_div(self):
        # float
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.div(2)
        assert t.getTable() == [0.5, 0.5, 0.5, 0.5]
        # list
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t.div([0.5, 2, 4, 8])
        assert t.getTable() == [2, 0.5, 0.25, 0.125]
        # table
        t = DataTable(size=4, chnls=1, init=[1, 1, 1, 1])
        t2 = DataTable(size=4, chnls=1, init=[0.5, 2, 4, 8])
        t.div(t2)
        assert t.getTable() == [2, 0.5, 0.25, 0.125]
        # list of list
        t = DataTable(size=4, chnls=2, init=[[1, 1, 1, 1], [1, 1, 1, 1]])
        t.div([[0.5, 2, 4, 8], [8, 4, 2, 0.5]])
        assert t.getTable(all=True) == [[2, 0.5, 0.25, 0.125], [0.125, 0.25, 0.5, 2]]

    def test_copyData(self):
        src = DataTable(size=8, chnls=1, init=[0, 1, 2, 3, 4, 5, 6, 7])
        dst = DataTable(size=8, chnls=1, init=[8, 9, 10, 11, 12, 13, 14, 15])
        dst.copyData(src)
        assert dst.getTable() == [0, 1, 2, 3, 4, 5, 6, 7]

        dst = DataTable(size=8, chnls=1, init=[8, 9, 10, 11, 12, 13, 14, 15])
        dst.copyData(src, 4, 0)
        assert dst.getTable() == [4, 5, 6, 7, 12, 13, 14, 15]

        dst = DataTable(size=8, chnls=1, init=[8, 9, 10, 11, 12, 13, 14, 15])
        dst.copyData(src, 0, 4)
        assert dst.getTable() == [8, 9, 10, 11, 0, 1, 2, 3]

        dst = DataTable(size=8, chnls=1, init=[8, 9, 10, 11, 12, 13, 14, 15])
        dst.copyData(src, 2, 4, 2)
        assert dst.getTable() == [8, 9, 10, 11, 2, 3, 14, 15]

        # Can take negative indices
        dst = DataTable(size=8, chnls=1, init=[8, 9, 10, 11, 12, 13, 14, 15])
        dst.copyData(src, -6, -4, 2)
        assert dst.getTable() == [8, 9, 10, 11, 2, 3, 14, 15]

    def test_rotate(self):
        t = DataTable(size=8, chnls=1, init=[0, 1, 2, 3, 4, 5, 6, 7])
        t.rotate(3)
        assert t.getTable() == [3, 4, 5, 6, 7, 0, 1, 2]

    def test_copy(self):
        t1 = DataTable(size=8, chnls=1, init=[0, 1, 2, 3, 4, 5, 6, 7])
        t2 = t1.copy()
        assert t1 != t2
        assert t1.getTable() == t2.getTable()

    def test_attr_size(self):
        t = HarmTable()
        assert t.size == 8192
        t.size = 512
        assert t.size == 512

@pytest.mark.usefixtures("audio_server")
class TestPyoMatrixObject:

    def test_isMatrixObject(self):
        a = Sine()
        assert not isMatrixObject(a)
        m = NewMatrix(width=4, height=2)
        assert isMatrixObject(m)

    def test_write(self, audio_server):
        path = os.path.join(os.path.expanduser("~"), "temporary_file.txt")

        m = NewMatrix(width=4, height=2, init=[[1,2,3,4],[5,6,7,8]])
        m.write(path)

        with open(path, "r") as f:
            values = eval(f.read())[0]

        assert len(values) == 2
        assert len(values[0]) == 4
        for j, sub in enumerate(values): # rows
            for i, val in enumerate(sub): # columns
                assert math.isclose(float(values[j][i]), m.get(i, j), abs_tol=0.0000001)

        os.remove(path)

    def test_read(self):
        path = os.path.join(os.path.expanduser("~"), "temporary_file.txt")

        width, height = 24, 16

        data = []
        for j in range(height):
            l = []
            for i in range(width):
                l.append(i + j)
            data.append(l)

        with open(path, "w") as f:
            f.write(str([data]))

        m = NewMatrix(width=width, height=height)
        m.read(path)

        assert m.getSize() == (width, height)

        for j in range(height):
            for i in range(width):
                # m.get() takes inverted arguments (maybe all matrix's logic needs review)
                assert m.get(i, j) == data[j][i]

        os.remove(path)

    def test_getSize(self):
        m = NewMatrix(width=4, height=2, init=[[1,2,3,4],[5,6,7,8]])
        assert m.getSize() == (4, 2)

    def test_normalize(self):
        width, height = 4, 2
        source = [[0.25, -0.25, 0.5, 0.25], [0. , 0.125, -0.125, -0.5]]
        expect = [[0.5, -0.5, 1, 0.5], [0. , 0.25, -0.25, -1]]
        m = NewMatrix(width=width, height=height, init=source)
        m.normalize(level=1)

        for j in range(height):
            for i in range(width):
                assert m.get(i, j) == expect[j][i]

    # TO BE IMPLEMENTED
    #def test_blur(self):

    # TO BE IMPLEMENTED
    #def test_boost(self):

    def test_put(self):
        m = NewMatrix(width=4, height=2, init=[[1,2,3,4],[5,6,7,8]])
        m.put(value=10, x=2, y=1)
        assert m.get(x=2, y=1) == 10

    def test_get(self):
        m = NewMatrix(width=4, height=2, init=[[1,2,3,4],[5,6,7,8]])
        assert m.get(x=2, y=1) == 7

    def test_getInterpolated(self):
        m = NewMatrix(width=4, height=2, init=[[1,2,3,4],[5,6,7,8]])
        assert m.getInterpolated(x=0.625, y=0.25) == 5.5


@pytest.mark.usefixtures("audio_server")
class TestPyoPVObject:

    def test_isPVObject(self):
        a = Sine()
        assert not isPVObject(a)
        pva = PVAnal(Sine(), 1024, 4)
        assert isPVObject(pva)

    def test_isPlaying(self):
        pva = PVAnal(Sine(), 1024, 4).stop()
        assert not pva.isPlaying()
        pva.play()
        assert pva.isPlaying()

        pva2 = PVAnal(Sine([1, 2]), 1024, 4).stop()
        assert not pva2.isPlaying()
        pva2.play()
        assert pva2.isPlaying(all=True) == [True, True]

    def test_play(self, audio_server):
        pva = PVAnal(Sine(), 1024, 4)
        assert pva.isPlaying() == True

        pva.stop()

        with Start(audio_server) as cxt:
            # wait 0.5 second before playing and play for 0.5 second
            pva.play(dur=0.5, delay=0.5)
            cxt.advance(0.25)
            assert pva.isPlaying() == False
            cxt.advance(0.5)
            assert pva.isPlaying() == True
            cxt.advance(0.5)
            assert pva.isPlaying() == False

    def test_stop(self, audio_server):
        pva = PVAnal(Sine(), 1024, 4)
        assert pva.isPlaying() == True

        pva.stop()
        assert pva.isPlaying() == False

        pva.play()

        with Start(audio_server) as cxt:
            pva.stop(wait=0.25)
            assert pva.isPlaying() == True
            cxt.advance(0.5)
            assert pva.isPlaying() == False

    def test_set(self, audio_server):
        pva = PVAnal(Sine(), 1024, 4)
        pvt = PVTranspose(pva, transpo=1)

        pvt.set(attr="transpo", value=0.5, port=0.3)

        with StartAndAdvance(audio_server, adv_time=0.5):
            assert pvt.transpo == 0.5

    def test_set_interupted(self, audio_server):
        pva = PVAnal(Sine(), 1024, 4)
        pvt = PVTranspose(pva, transpo=1)

        pvt.set(attr="transpo", value=0.5, port=0.5)

        with StartAndAdvance(audio_server, adv_time=0.25) as cxt:
            pvt.set(attr="transpo", value=0.25, port=0.3)
            cxt.advance(0.5)
            assert pvt.transpo == 0.25


@pytest.mark.usefixtures("audio_server")
class TestMix:

    def test_init_one_to_many(self):
        a = Sine()
        b = Mix(a, voices=2)
        assert len(b) == 2

    def test_init_many_to_one(self):
        a = Sine([100, 200])
        b = Mix(a, voices=1)
        assert len(b) == 1

    def test_input_list(self, audio_server):
        a = Sig([100, 200])
        b = Sig([300, 400])
        c = Mix([a, b], voices=2)

        with StartAndAdvanceOneBuf(audio_server):
            assert c.get(all=True) == [400, 600]

    def test_invalid_voices(self, audio_server):
        a = Sig([100, 200])
        b = Mix(a, voices=-1)
        assert len(b) == 1

        with StartAndAdvanceOneBuf(audio_server):
            assert b.get() == 300

    def test_lmax_bigger_than_input_length(self, audio_server):
        a = Sig([100, 200])
        b = Mix(a, voices=3, mul=[1, 1, 0.5])
        assert len(b) == 3

        with StartAndAdvanceOneBuf(audio_server):
            assert b.get(all=True)[0] == 100
            assert b.get(all=True)[1] == 200
            assert b.get(all=True)[2] == 50


@pytest.mark.usefixtures("audio_server")
class TestDummy:

    def test_dummy(self, audio_server):
        a = Sig(1)
        b = Sig(2)
        c = Sig(3)

        d = a + b + c

        with StartAndAdvanceOneBuf(audio_server):
            assert d.get() == 6


@pytest.mark.usefixtures("audio_server")
class TestInputFader:

    def test_setInput(self, audio_server):
        a = InputFader(Sig(1))

        with StartAndAdvanceOneBuf(audio_server) as cxt:
            assert a.get() == 1
            a.setInput(Sig(2), fadetime=0.005)
            cxt.advance(0.01)
            assert a.get() == 2

    def test_attr_input(self, audio_server):
        a = InputFader(Sig(1))

        with StartAndAdvanceOneBuf(audio_server) as cxt:
            assert a.get() == 1
            a.input = Sig(2)
            cxt.advance(0.06)
            assert isinstance(a.input, Sig)
            assert a.get() == 2


@pytest.mark.usefixtures("audio_server")
class TestSig:

    def test_setValue(self, audio_server):
        a = Sig(0)
        a.setValue(1)

        with StartAndAdvanceOneBuf(audio_server) as cxt:
            assert a.get() == 1
            a.value = 2
            cxt.advanceOneBuf()
            assert a.get() == 2
            assert a.value == 2


@pytest.mark.usefixtures("audio_server")
class TestVarPort:

    def test_setValue(self, audio_server):
        a = VarPort(value=1, time=0.5, init=0)

        with StartAndAdvance(audio_server, adv_time=0.25) as cxt:
            a.value = 2
            cxt.advance(0.3)
            assert a.get() != 2
            cxt.advance(0.3)
            assert a.get() == 2
            assert a.value == 2

    def test_setTime(self, audio_server):
        a = VarPort(value=1, time=0.5, init=0)

        with StartAndAdvance(audio_server, adv_time=0.25) as cxt:
            a.time = 1
            cxt.advance(0.5)
            assert a.get() < 1
            cxt.advance(0.6)
            assert a.get() == 1
            assert a.time == 1

    def test_setFunction(self, audio_server):
        func1 = mock.Mock()
        func2 = mock.Mock()

        a = VarPort(value=1, time=0.5, init=0, function=func1)

        with Start(audio_server) as cxt:
            cxt.advance(0.3)

            a.function = func2

            cxt.advance(0.4)

            assert not func1.called
            func2.assert_called_once()
            assert a.function == func2


@pytest.mark.usefixtures("audio_server")
class TestPow:

    def test_setBase(self, audio_server):
        a1 = 2
        a2 = Sig(2)

        p = Pow(exponent=16)
        p.base = a1

        with StartAndAdvanceOneBuf(audio_server) as cxt:
            assert p.base == 2
            assert p.get() == 65536
            p.base = a2
            cxt.advanceOneBuf()
            assert p.base.value == 2
            assert p.get() == 65536

    def test_setExponent(self, audio_server):
        a1 = 16
        a2 = Sig(16)

        p = Pow(base=2)
        p.exponent = a1

        with StartAndAdvanceOneBuf(audio_server) as cxt:
            assert p.exponent == 16
            assert p.get() == 65536
            p.exponent = a2
            cxt.advanceOneBuf()
            assert p.exponent.value == 16
            assert p.get() == 65536

# Can I define proto to automatically test attribute setter and getter and combination of `a` and `i` attributes ?
