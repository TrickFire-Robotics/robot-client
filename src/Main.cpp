#include "Main.h"

#define COL1 128
#define COL2 300

#define ROW1 8
#define ROW2 64
#define ROW3 ROW2 + 264 + 16

namespace trickfire {

Command * drivebase;
StandardDriveCommand standardDrive;
AutoDriveCommand1 autoDrive1;
ArucoDriveCommand arucoDrive;

void Main::Start() {
	Logger::SetLoggingLevel(Logger::LEVEL_INFO_FINE);
	RobotIO::Start();

	Client client(SERVER_IP, 25565);
	sf::Mutex mut_client;
	client.SetMessageCallback(Main::OnClientMessageReceived);

	drivebase = &standardDrive;
	standardDrive.Start();

#if OPENCV
#if CAMERA
	CameraSendCommand cameraCommand(&client, &mut_client, &arucoDrive);
	cameraCommand.Start();
#endif
#if KINECT
	Freenect::Freenect freenect;

	TFFreenect& temp = freenect.createDevice<TFFreenect>(0);
	temp.startVideo();
	temp.stopVideo();
	freenect.deleteDevice(0);

	TFFreenect& kinect = freenect.createDevice<TFFreenect>(0);
	kinect.setLed(LED_GREEN);
	KinectSendCommand kinectCommand(&client, &mut_client, kinect);
	kinectCommand.Start();
	kinect.setTiltDegrees(0);
	kinect.updateState();
#endif
#endif

#if defined(GUI_ENABLED) and GUI_ENABLED
	sf::Thread windowThread(SfmlWindowThread);
	windowThread.launch();
	windowThread.wait();
#else
	client.Join();
#endif

	drivebase->Stop();
	Command::KillAll();

	client.Disconnect();
	RobotIO::Stop();

#if KINECT
	kinect.stopVideo();
	kinect.stopDepth();
	kinect.setLed(LED_RED);
#endif
}

void Main::ResumeStandardDrive() {
	if (drivebase != &standardDrive) {
		drivebase->Stop();
		drivebase = &standardDrive;
		drivebase->Start();
	}
}

void Main::OnClientMessageReceived(Packet& packet) {
	int type = -1;
	packet >> type;
	switch (type) {
	case DRIVE_PACKET:
		Logger::Log(Logger::LEVEL_INFO_VERY_FINE, "Received drive packet");
		double forwards;
		double rotation;
		packet >> forwards >> rotation;
		standardDrive.SetVals(forwards, rotation);
		break;
	case AUTO_PACKET_1:
		Logger::Log(Logger::LEVEL_INFO_VERY_FINE, "Received auto 1 packet");
		if (drivebase != &arucoDrive) {
			drivebase->Stop();
			Logger::Log(Logger::LEVEL_INFO_FINE, "Starting ArUco drive command");
			drivebase = &arucoDrive;
			drivebase->Start();
		}
		break;
	default:
		Logger::Log(Logger::LEVEL_INFO_FINE, "Received unknown packet (type " + to_string(type) + ")");
		break;
	}
}

void Main::SfmlWindowThread() {
	Font wlmCarton;
	if (!wlmCarton.loadFromFile("wlm_carton.ttf")) {
		Logger::Log(Logger::LEVEL_ERROR, "Error loading font: will continue but text will be invisible");
	}

	RenderWindow window(VideoMode(500, 768), "TrickFire Robotics - Client");

	while (window.isOpen()) {
		window.clear(Color::Black);

		Event event;

		while (window.pollEvent(event)) {
			if (event.type == Event::Closed) {
				window.close();
			}
		}

		DrawTrickFireHeader(wlmCarton, window);

		Color background(64, 64, 64);

		Vector2f forwLabelSize = DrawingUtil::DrawGenericHeader("Rot.",
				Vector2f(COL1, ROW1), false, wlmCarton, Color::Green, window);
		DrawingUtil::DrawCenteredAxisBar(DisplayVariables::GetRot(),
				Vector2f(COL1 + (forwLabelSize.x / 2) - 20, ROW2),
				Vector2f(40, 264), Vector2f(4, 4), background, Color::Green,
				window);

		Vector2f rotLabelSize = DrawingUtil::DrawGenericHeader("Drv.",
				Vector2f(COL2, ROW1), false, wlmCarton, Color::Green, window);
		DrawingUtil::DrawCenteredAxisBar(DisplayVariables::GetDrive(),
				Vector2f(COL2 + (rotLabelSize.x / 2) - 20, ROW2),
				Vector2f(40, 264), Vector2f(4, 4), background, Color::Green,
				window);

		window.display();
	}
}

void Main::DrawTrickFireHeader(Font& font, RenderWindow& window) {
	Text header;
	header.setFont(font);
	header.setCharacterSize(60);
	header.setColor(Color::Green);
	header.setStyle(Text::Italic);
	header.setString("TrickFire Robot Client");
	header.setOrigin(0, 2 * header.getLocalBounds().height);
	header.setRotation(90);
	window.draw(header);
}
}
