#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <Windows.h>

using namespace std;
using namespace cv;

Mat hwnd2mat(HWND hwnd);
void MouseMove(int x, int y);

int main(int, char **)
{
	const Scalar CV_BLACK = Scalar(0, 0, 0);
	const Mat element = getStructuringElement(MORPH_RECT, Size(3, 3), Point(1, 1));

	SetProcessDPIAware();
	setUseOptimized(true);
	auto hwndHighFleet = FindWindowEx(NULL, NULL, NULL, TEXT("HIGHFLEET v.1.151"));

	Mat src, tmp1, tmp2, tmp3;

	// video loop
	int key = 0;
	Point player_lastframe = Point(-1, -1);
	Point enemy_lastframe = Point(-1, -1);

	while (key != 27) // wait for escape key
	{
		src = hwnd2mat(hwndHighFleet);
		tmp1 = src.clone();
		// extractChannel(src, tmp1, 1); // optimisation: we only need the G channel
		// black out the UI rectangles in the bottom left/right corners
		rectangle(tmp1, Point(0, 780), Point(349, 1079), CV_BLACK, FILLED);
		rectangle(tmp1, Point(1570, 780), Point(1919, 1079), CV_BLACK, FILLED);
		// rectangle(tmp1, Point(928, 88), Point(989, 101), CV_BLACK, FILLED); // black out the "HONOR" text

		// filter to valid green values
		inRange(tmp1, Scalar(0, 252, 0, 0), Scalar(254, 255, 255, 255), tmp2);
		// remove small lines
		morphologyEx(tmp2, tmp3, MORPH_OPEN, element);

		// iterate through contours to find the player and enemy
		vector<vector<Point>> contours;
		vector<Vec4i> hierarchy;
		findContours(tmp3, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE);
		// Mat dst = Mat::zeros(tmp3.size(), CV_8UC3);
		int closest_diff = 10; // max tolerance of 10 pixels difference
		int closest_i = -1;
		int closest_player = INT_MAX;
		Point player = Point(-1, -1);
		Point enemy = Point(-1, -1);
		for (size_t i = 0; i < contours.size(); i++)
		{
			RNG rng(i);
			Rect r = boundingRect(contours[i]);
			Scalar color = Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256));

			// player engine heat bar should be 5 pixels tall and rectangular
			if (r.height == 5 && contours[i].size() == 4)
			{
				Point player_candidate = Point(r.x + r.width / 2, r.y + 50);
				// sometimes the player marker jumps around so we only want the one closest to the previous frame
				int player_diff = abs(player_candidate.x - player_lastframe.x) + abs(player_candidate.y - player_lastframe.y);
				if (player_diff < closest_player)
				{
					closest_player = player_diff;
					player = player_candidate;
				}
			}

			// contour with bounding box closest to 62x62 is the enemy marker (see max tolerance above)
			int diff = abs(r.width - 62) + abs(r.height - 62);
			if (diff < closest_diff)
			{
				closest_diff = diff;
				closest_i = i;
				enemy = Point(r.x + 31, r.y + 31);
			}
			// drawContours(dst, contours, (int)i, color, 1, LINE_8, hierarchy, 0);
		}

		// mark player if found
		if (player.x != -1)
		{
			circle(src, player, 10, Scalar(255, 255, 0), 2);
		}

		// mark enemy if found
		if (enemy.x != -1)
		{
			circle(src, enemy, 10, Scalar(0, 0, 255), 2);
		}

		// skip processing if this is indentical to last frame
		// I can't be bothered trying to sync the loop to the game FPS so I just check the frame is unique instead
		if (player == player_lastframe || enemy == enemy_lastframe)
		{
			goto skipproc;
		}

		// do ballistics equations if position data is available for current and previous frame
		// units in pixels per frame (px/fr) I guess
		// 180mm velocity: 10 px/fr
		// 37mm velocity: 16 px/fr
		// then draw ideal angle on window for player
		if (player.x != -1 && enemy.x != -1 && player_lastframe.x != -1 && enemy_lastframe.x != -1)
		{
			Point2f displacement = enemy - player;
			Point2f velocity = (enemy - enemy_lastframe) - (player - player_lastframe);

			// funky ballistics equation time, see https://stackoverflow.com/questions/51851120/2d-target-intercept-algorithm
			float dotproduct = velocity.dot(displacement);
			int distsquared = displacement.x * displacement.x + displacement.y * displacement.y;
			float u2 = velocity.x * velocity.x + velocity.y * velocity.y;
			float v2 = 10 * 10; // using speed for 180mm ammo
			float t = -1;		// collision time, initialised to invalid
			float v2_u2 = v2 - u2;

			if (u2 == v2)
			{
				t = distsquared / (dotproduct * 2);
			}
			else
			{
				float discriminant = dotproduct * dotproduct + distsquared * v2_u2;

				// real roots exist
				if (discriminant > 0.0f)
				{
					float sqrtdisc = sqrt(discriminant);
					// I have no clue whats happening here, but I guess its getting the smaller of both roots?
					if (abs(dotproduct) < sqrtdisc)
						t = (dotproduct + sqrtdisc) / v2_u2;
					else if (dotproduct > 0.0f)
						t = (dotproduct - sqrtdisc) / v2_u2;
				}
			}

			if (t != -1)
			{
				Point projectile = displacement + velocity * t;
				cv::line(src, player, (projectile + player), Scalar(255, 255, 0), 1);

				// move the mouse if left ctrl is held
				if (GetAsyncKeyState(VK_LCONTROL) != 0)
				{
					RECT rc;
					GetWindowRect(hwndHighFleet, &rc);
					// center of screen according to game is here for some reason
					int screenx = rc.left + 658 + projectile.x / 20;
					int screeny = rc.top + 422 + projectile.y / 20;
					MouseMove(screenx, screeny);
				}
			}
		}

		player_lastframe = player;
		enemy_lastframe = enemy;

	skipproc:

		/**
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hwndHighFleet, &p);
		circle(src, Point(p.x, p.y), 10, Scalar(0, 255, 0));
		**/

		imshow("Preview", src);

		// does this ordering matter? I have no clue
		key = waitKey(1);
	}
}

Mat hwnd2mat(HWND hwnd)
{

	HDC hwindowDC, hwindowCompatibleDC;

	int height, width, srcheight, srcwidth;
	HBITMAP hbwindow;
	Mat src;
	BITMAPINFOHEADER bi;

	hwindowDC = GetDC(hwnd);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	RECT windowsize; // get the height and width of the screen
	GetClientRect(hwnd, &windowsize);

	srcheight = windowsize.bottom; // change this to whatever size you want to resize to
	srcwidth = windowsize.right;
	height = windowsize.bottom; // change this to whatever size you want to resize to
	width = windowsize.right;
	// height = windowsize.bottom * 0.3125; // change this to whatever size you want to resize to
	// width = windowsize.right * 0.625;

	src.create(height, width, CV_8UC4);

	// create a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
	bi.biSize = sizeof(BITMAPINFOHEADER); // http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
	bi.biWidth = width;
	bi.biHeight = -height; // this is the line that makes it draw upside down or not
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	// use the previously created device context with the bitmap
	SelectObject(hwindowCompatibleDC, hbwindow);
	// copy from the window device context to the bitmap device context
	StretchBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, 0, 0, srcwidth, srcheight, SRCCOPY); // change SRCCOPY to NOTSRCCOPY for wacky colors !
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, src.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);	 // copy from hwindowCompatibleDC to hbwindow

	// avoid memory leak
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(hwnd, hwindowDC);

	return src;
}

void MouseMove(int x, int y)
{
	double fScreenWidth = GetSystemMetrics(SM_CXSCREEN) - 1;
	double fScreenHeight = GetSystemMetrics(SM_CYSCREEN) - 1;
	double fx = x * (65535.0f / fScreenWidth);
	double fy = y * (65535.0f / fScreenHeight);
	INPUT Input = {0};
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	Input.mi.dx = (long)fx;
	Input.mi.dy = (long)fy;
	SendInput(1, &Input, sizeof(INPUT));
}