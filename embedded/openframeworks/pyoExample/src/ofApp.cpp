#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // define audio properties
    int sampleRate = 48000;
    int bufferSize = 256;
    int nChannels = 2;

	exiting = false;

	// font stuff
	ofTrueTypeFont::setGlobalDpi(72);
	font.load("DroidSansMono.ttf", 14, true, true);
	interpreterRectHeight = font.stringHeight("q");
	letterWidth = font.stringWidth("a");
	interpreterRectHeight += (interpreterRectHeight - font.stringHeight("b"));
	interpreterRectWidth = letterWidth * 80;
	interpreterRectXPos = 100;
	interpreterRectYPos = 100+(interpreterRectHeight*0.2);
	cursorTimeStamp = ofGetElapsedTimeMillis();
	showCursor = true;
	cursorXPos = interpreterRectXPos + (letterWidth / 2);
	cursorYPos = interpreterRectYPos + 2;
	cursorHeight = interpreterRectHeight - 4;

    // initialize a pyo server
    pyo.setup(nChannels, nChannels, bufferSize, sampleRate);
	// get the STDOUT to not print the message about wxPython
	pyo.getStdout();
	// set the debug state to active
	pyo.setDebug(1);
    // load a python file
    int err = pyo.loadfile("data/scripts/stereoDelay.py", 0);
	if (err) {
		if (err == 1) {
			stderrStr << "could not load file";
		}
		else {
			stderrStr << pyo.getErrorMsg();
		}
	}
    // initialize OpenFrameworks audio streaming channels
	// soundStream.printDeviceList();     // Uncomment if you need to
    // soundStream.setDeviceID(1);   // change the audio device.
	ofSoundStreamSettings settings;
	settings.numOutputChannels = nChannels;
	settings.sampleRate = sampleRate;
	settings.bufferSize = bufferSize;
	settings.numBuffers = 4;
	settings.setOutListener(this);
	soundStream.setup(settings);
}

//--------------------------------------------------------------
void ofApp::update(){
	unsigned long timeStamp = ofGetElapsedTimeMillis();
	if (timeStamp - cursorTimeStamp > CURSORBLINK) {
		showCursor = !showCursor;
		cursorTimeStamp = timeStamp;
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofSetColor(ofColor::white);
	font.drawString("Interpreter\n(Type \"st_rev.stop()\" to stop the sound)", interpreterRectXPos, interpreterRectYPos-(interpreterRectHeight*0.4)-cursorHeight);
	ofDrawRectangle(interpreterRectXPos, interpreterRectYPos, interpreterRectWidth, interpreterRectHeight);

	ofSetColor(ofColor::black);
	if (showCursor) {
		ofDrawLine(cursorXPos+(typeStr.size()*letterWidth), cursorYPos, cursorXPos+(typeStr.size()*letterWidth), cursorYPos+cursorHeight);
	}
	if (typeStr.size() > 0) {
		font.drawString(typeStr, cursorXPos, cursorYPos+cursorHeight-2);
	}
	if (stdoutStr.size() > 0) {
		ofSetColor(ofColor::white);
		font.drawString(stdoutStr, cursorXPos, cursorYPos+(cursorHeight*2));
	}
	else if (stderrStr.size() > 0) {
		ofSetColor(ofColor::red);
		font.drawString(stderrStr, cursorXPos, cursorYPos+(cursorHeight*2));
	}
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer & buffer){
    if (!exiting) pyo.fillin(&buffer[0]);
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer & buffer){
	if (!exiting) pyo.process(&buffer[0]);
}

//--------------------------------------------------------------
void ofApp::exit(){
	while (pyo.isProcessing());
	exiting = true;
	ofExit();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	if (key == OF_KEY_DEL || key == OF_KEY_BACKSPACE) {
		typeStr = typeStr.substr(0, typeStr.length()-1);
	}
	else if (key == OF_KEY_RETURN) {
		int err = pyo.exec(typeStr.c_str());
		stdoutStr.clear();
		stderrStr.clear();
		if (err) {
			stderrStr = pyo.getErrorMsg();
		}
		else {
			// if there's anything to be printed in the console, it can be retrieved as a vector of strings
			// with pyo.getStdout()
			// if this vector contains any elements, then the Python interpreter has printed something
			std::vector<std::string> v = pyo.getStdout();
			if (v.size() > 0) {
				// the last item of the vector is a newline character, so we don't print it
				// hence i < v.size()-1
				for (size_t i = 0; i < v.size()-1; i++) {
					stdoutStr += v[i];
				}
			}
		}
		typeStr.clear();
	}
	else if (key == OF_KEY_SHIFT || key == OF_KEY_CONTROL || key == OF_KEY_ALT
			|| (key >= 3680 && key <= 3685)) {
		// do nothing on shift, ctrl or alt keys
	}
	else {
		typeStr += char(key);
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){

}
