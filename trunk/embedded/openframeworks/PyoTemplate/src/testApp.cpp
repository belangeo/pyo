#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){
    // define audio properties
    int sampleRate = 44100;
    int bufferSize = 256;
    int nChannels = 2;
    // initialize a pyo server
    pyo.setup(nChannels, bufferSize, sampleRate);
    // load a python file
    pyo.loadfile("../scripts/stereoDelay.py", 0);
    // initialize OpenFrameworks audio streaming channels
	soundStream.setup(this, nChannels, nChannels, sampleRate, bufferSize, 4);
}

void testApp::audioIn(float * input, int bufferSize, int nChannels){
    // send audio samples to pyo
    pyo.fillin(input);
}

void testApp::audioOut(float * output, int bufferSize, int nChannels){
    // process and get new audio samples from pyo
    pyo.process(output);
}

//--------------------------------------------------------------
void testApp::update(){

}

//--------------------------------------------------------------
void testApp::draw(){

}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){

}
