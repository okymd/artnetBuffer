#pragma once

#include "ofMain.h"
#include "ofxUdpManager.h"
#include "ofxXmlSettings.h"
#include "ofxGui.h"
#include "ofxOsc.h"

#define ART_NET_ID "Art-Net\0"
#define ART_DMX 0x5000
#define ART_DMX_START 18


static const int ARTNET_PORT = 6454; 
static const unsigned int ARTNET_PACKET_SIZE = 530;// Art-net packet size

//LED Devices
static const unsigned int NUM_LEDS = 192;
static const unsigned int PACKET_SIZE = NUM_LEDS * 3;// for Arduino packet size
static const int NUM_REMOTE_DEVICES = 5;

//
static const int MAX_NUM_UNIVERSES = NUM_REMOTE_DEVICES*2;
	
//OSC 
static const string HOST  = "localhost";
static const int OSC_PORT = 2256;

class ofApp : public ofBaseApp{

	public:
		void setup();
		void exit();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);



	public:
		char * frameBuffer; //
		char * sendBuffer[NUM_REMOTE_DEVICES];
		ofxUDPManager udpSender[NUM_REMOTE_DEVICES];
		ofxUDPManager udpReceiver;

		
				
		unsigned int numRecvUniverses;
	
		enum {
			REC,
			PLAY,
		};
		int mode;

		static const unsigned int MAX_FRAME_NUM = 1024;
		static const unsigned int FRAME_SIZE = ARTNET_PACKET_SIZE*MAX_NUM_UNIVERSES;

		vector<char*> frames;
		
		void storePacket(const char* artnetPacket);
		void recallFrame(int frameNum);
		void storeFrame(char* frameBuffer,int frameNum);
		
		void sendPacket(int index,char* artnetPacket);
		void sendFrame(char* frameBuffer);


		void allocateFrameBuffer();
		void releaseFrameBuffer();

		void receivePacket(char* artnetPacket);
		void parseArtnetDMX(char* artnetPacket);
		void sendTestPacket(int index,ofColor color);
		int  getUniverse(const char* artnetPacket);
		//
		bool bReceive[MAX_NUM_UNIVERSES];
		bool bSend[MAX_NUM_UNIVERSES];


	//ofParamer
		ofParameter<int> lastFrame;
		ofParameter<string> currentFps;
		ofxIntSlider currentFrame;
		ofxIntSlider endFrame;
		ofxIntSlider startFrame;
		ofxIntSlider fps;
		ofxIntSlider bright;
		
	//GUI
		ofxPanel gui;
		ofxGuiGroup guiPB;
		ofxToggle bRecording;
		ofxToggle bPlaying;
		ofxToggle bThrough;
		ofxToggle bPause;
		ofxButton btnTriple;
		ofxButton btnDouble;
		ofxButton btnNormal;
		ofxButton btnTest;
		ofxButton btnReconnect;
		
		ofxToggle bShowMask; 
		ofxToggle bApplyMask;
		

		ofxLabel  labelStatus;
		ofxLabel  labelFPS;
		

	//GUI handler
		void onRec(bool &bRec);
		void onPlay(bool &bPlay);
		void onChangeFPS(int &val);
		void onChangeBright(int &bval);
		void onTriple(){ fps=75;};
		void onDouble(){ fps=50;};
		void onNormal(){ fps=25;};
		void onTest();
		void onReconnect();


	//OSC
	ofxOscSender oscSender[NUM_REMOTE_DEVICES];


	//image mask
	ofxLabel maskStatus;
	ofImage sendImg;
	ofImage maskImg[NUM_REMOTE_DEVICES];
	int currentShowMask;
	void updateSendPixel();
	void drawPixel(char *pixel);
	void doMask(char * src,char* mask,char* dst);
	//active Pixel
	int getActive(int x,int y);
	int activePixel;

	//draw Help
	ofTrueTypeFont font;
	void drawKeymap();

};
