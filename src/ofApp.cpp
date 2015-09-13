#include "ofApp.h"


////Arduino IP's
const string remoteIP[NUM_REMOTE_DEVICES] ={
	"192.168.1.200",
	"192.168.1.201",
	"192.168.1.202",
	"192.168.1.203",
	"192.168.1.204"
};

////Arduino IP's
//const string remoteIP[5] ={
//	"192.168.0.200",
//	"192.168.0.201",
//	"192.168.0.202",
//	"192.168.0.203",
//	"192.168.0.204"
//};


//--------------------------------------------------------------
void ofApp::setup(){

	//Receiver Setup
	udpReceiver.Create();
	udpReceiver.Bind(ARTNET_PORT);
	udpReceiver.SetNonBlocking(true);


	printf("%d\n",udpReceiver.GetReceiveBufferSize());


	//Sender Setup

	for(int i=0;i<NUM_REMOTE_DEVICES;i<i++){
		udpSender[i].Create();
		udpSender[i].Connect(remoteIP[i].c_str(),ARTNET_PORT);
		udpSender[i].SetNonBlocking(true);
	}

	buffer = new char[ARTNET_PACKET_SIZE];

	allocateBuffer();

	mode = THROUGH;
	maxNumUniverses=0;

	//gui setup
	btnTriple.addListener(this,&ofApp::onTriple);
	btnDouble.addListener(this,&ofApp::onDouble);
	btnNormal.addListener(this,&ofApp::onNormal);
	fps.addListener(this,&ofApp::onChangeFPS);
	bRecording.addListener(this,&ofApp::onRec);
	bPlaying.addListener(this,&ofApp::onPlay);
	btnTest.addListener(this,&ofApp::onTest);

	ofxGuiSetTextPadding(4);
	ofxGuiSetDefaultWidth(ofGetWindowWidth());

	gui.setup();
	gui.setPosition(0,60);
	gui.add(labelStatus.setup(status));
	gui.add(bRecording.setup("Rec",false));
	gui.add(bPlaying.setup("Play",false));
	gui.add(currentFrame.setup("Frame",0,0,MAX_FRAME_NUM));
	gui.add(maxFrame.setup("Max Frame",MAX_FRAME_NUM,1,MAX_FRAME_NUM));
	gui.add(btnTriple.setup("FPSx3"));
	gui.add(btnDouble.setup("FPSx2"));
	gui.add(btnNormal.setup("FPSx1"));
	gui.add(fps.setup("FPS",75,25,100));
	gui.add(btnTest.setup("TEST"));

	


}
void ofApp::exit(){
	releaseBuffer();
}

//--------------------------------------------------------------
void ofApp::update(){
	ofSetWindowTitle(ofToString(ofGetFrameRate(), 2));


	static unsigned long start=0;

	for(int i=0;i<MAX_NUM_UNIVERSES;i++){
		bSend[i]=false;
		bReceive[i]=false;
	}

	if(mode==PLAY){

		status="PLAY";
		for(int i=0;i<maxNumUniverses;i++){
			currentFrame = currentFrame+1;
			if(currentFrame>maxFrame)
				currentFrame = 0;
			sendPacket(frames[currentFrame]);
		}
	
	}
	
	if(mode==THROUGH || mode==REC){

		//start=ofGetElapsedTimeMicros();
		//status = "No Art-net receive";

		int ret;
		numUniverses = 0;


		while(ret=udpReceiver.Receive(buffer,ARTNET_PACKET_SIZE) >= 0){
			//parseArtnetDMX(buffer);
			receivePacket(buffer);
			sendPacket(buffer);

			if(bRecording){
				
				currentFrame = currentFrame+1;
				if(currentFrame>maxFrame-1){
					bRecording=false;
				}
				storeFrame(buffer,currentFrame);
				numUniverses++;
				if(maxNumUniverses<numUniverses)maxNumUniverses=numUniverses;

			}else status="THROUGH";
		}		
		//printf("%d\n",ofGetElapsedTimeMicros()-start);
	}
}

void ofApp::storeFrame(const char* buffer,int frame){
	memcpy(frames[frame],buffer,ARTNET_PACKET_SIZE);
}

void ofApp::allocateBuffer(){
	
	for(int i=0;i<MAX_FRAME_NUM;i++){
		char* frame = new char[530];
		frames.push_back(frame);
	}
}
void ofApp::releaseBuffer(){
	
	for(int i=0;i<frames.size();i++){
		delete frames[i];
	}
}


void ofApp::parseArtnetDMX(char* artnetPacket){


		// Check that packetID is "Art-Net" else ignore
		for (int i = 0 ; i < 9 ; i++)
		{
			if (artnetPacket[i] != ART_NET_ID[i])
				return ;
		}
        
		int opcode = artnetPacket[8] | artnetPacket[9] << 8; 
		int sequence;
		int incomingUniverse;
		int dmxDataLength;
		char* data = artnetPacket+ ART_DMX_START;

		if (opcode == ART_DMX){

			sequence = artnetPacket[12];
			incomingUniverse = artnetPacket[14] | artnetPacket[15] << 8;  
			dmxDataLength = artnetPacket[17] | artnetPacket[16] << 8;
			
		}

		//printf("Sequence:%3d\tUniverse:%2d\n",sequence,incomingUniverse);

		bReceive[incomingUniverse]=true;

}
int ofApp::getUniverse(const char* artnetPacket){
	return artnetPacket[14] | artnetPacket[15] << 8 ;
}


void ofApp::receivePacket(char* artnetPacket){
	bReceive[getUniverse(buffer)]=true;
}

void ofApp::sendPacket(char* artnetPacket){
	
	char buffer[ARTNET_PACKET_SIZE];
	memcpy(buffer,artnetPacket,ARTNET_PACKET_SIZE);
	int universe = getUniverse(buffer);
	int index = universe/2 ;
	int sendUniverse = universe%2;
	buffer[14] = sendUniverse ;
	udpSender[index].SendAll(buffer,ARTNET_PACKET_SIZE);
	bSend[universe]=true;
}

void ofApp::sendTestPacket(int universe,ofColor color){
	
	char artnetPacket[ARTNET_PACKET_SIZE];

	// packetID is "Art-Net"
	for (int i = 0 ; i < 9 ; i++)
		artnetPacket[i] = ART_NET_ID[i];
	
	//opcode
	artnetPacket[8] = (char)(ART_DMX & 0xff00) ;
	artnetPacket[9] = (char)((ART_DMX & 0x00ff) >> 8);
	//sequence
	artnetPacket[12]= 0;
	//universe
	artnetPacket[14]= (char)(universe & 0xff00);
	artnetPacket[15]= (char)(universe & 0x00ff) >> 8;
	
	//
	artnetPacket[17] = (char)512 & 0x00ff;
	artnetPacket[18] = (char)((512 & 0xff00) >> 8);

	for(int i=ART_DMX_START;i<=512;i++){
		
		char v;
		if(i%3==0)v=color.r;
		else if(i%3==1)v=color.g;
		else if(i%3==2)v=color.b;

		artnetPacket[i]=v;
	}
	
	sendPacket(artnetPacket);

	for(int i=0;i<ART_DMX_START-1;i++){
		printf("[%d] %x\n",i,artnetPacket[i]);
	}

	//for(int i=ART_DMX_START;i<ARTNET_PACKET_SIZE;i++){
	//	printf("[%d] %x\n",i,artnetPacket[i]);
	//}
}

void ofApp::onTest(){

	sendTestPacket(0,ofColor(30,30,30));

	//for(int i=0;i<10;i++){
	//	sendTestPacket(i,ofColor(30,30,30));
	//}
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(ofColor(0,0,0));

	for(int i=0;i<MAX_NUM_UNIVERSES;i++){

		if(bReceive[i])ofSetColor(ofColor(0,0,200));
		else ofSetColor(ofColor(30));
		ofCircle(ofPoint(15*(i+1),15*1),5);

		if(bSend[i])ofSetColor(ofColor(0,200,0));
		else ofSetColor(ofColor(30));
		ofCircle(ofPoint(15*(i+1),15*2),5);
		
	}
	
	gui.draw();
	
}

void ofApp::onRec(bool &bRec){

	

	//on Recoding
	if(bRec){
		mode=REC;
		currentFrame.setFillColor(ofColor(200,0,0));

	}else{
		mode=THROUGH;
		currentFrame.setFillColor(ofColor(120));
		int max = currentFrame;
		maxFrame = max;
		currentFrame=0;
	}

	if(bPlaying)bPlaying=false;
}

void ofApp::onPlay(bool &bPlay){

	if(bRecording)bRecording=false;

	//on Play
	if(bPlay){
		mode=PLAY;
		currentFrame.setFillColor(ofColor(0,200,0));

	}else{
		mode=THROUGH;
		currentFrame.setFillColor(ofColor(120));
		currentFrame=0;
	}
}


void ofApp::onChangeFPS(int &val){

	ofSetFrameRate(val);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){


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
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
