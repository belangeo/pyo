from pyo import *

class FreqMod:
    def __init__(self, carrier=250, ratio=.5, index=1, amplitude=1):
        self.carrierFrequency = carrier
        self.ratio = ratio
        self.index = index
        self.modulatorFrequency = carrier * ratio
        self.modulatorAmplitude = self.modulatorFrequency * index
        self.amplitude = amplitude
        
        self.table = HarmTable([1])
        self.modulator = Osc(self.table, self.modulatorFrequency, 0, self.modulatorAmplitude, self.carrierFrequency)
        self.carrier = Osc(self.table, self.modulator, 0, self.amplitude)
        
    def play(self):
        self.modulator.play()
        self.carrier.out()
        return self
        
    def stop(self):
        self.modulator.stop()
        self.carrier.stop()

    def setCarrier(self, carrier):
        self.carrierFrequency = carrier
        self.modulatorFrequency = carrier * self.ratio
        self.modulatorAmplitude = self.modulatorFrequency * self.index
        self.refresh()

    def setRatio(self, ratio):
        self.ratio = ratio
        self.modulatorFrequency = self.carrierFrequency * ratio
        self.modulatorAmplitude = self.modulatorFrequency * self.index
        self.refresh()

    def setIndex(self, index):
        self.index = index
        self.modulatorAmplitude = self.modulatorFrequency * index
        self.refresh()
          
    def refresh(self):    
        self.modulator.setFreq(self.modulatorFrequency) 
        self.modulator.setMul(self.modulatorAmplitude)
        self.modulator.setAdd(self.carrierFrequency)       
