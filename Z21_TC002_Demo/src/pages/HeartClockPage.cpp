#include "pages/HeartClockPage.h"
#include "utils/Painter.h"
#include "base/base.h"
#include <ctime>
#include <cstdio>

namespace {

#define DISPLAY_WIDTH  52
#define DISPLAY_HEIGHT 16
#define BEIJING_UTC_OFFSET_SECONDS (8 * 60 * 60)

static const Color COLOR_BLACK(0, 0, 0);
static const Color COLOR_DIM_RED(90, 0, 8);
static const Color COLOR_RED(255, 0, 24);
static const Color COLOR_WHITE(220, 220, 220);
static const Color COLOR_BLUE(0, 70, 160);

const char* DIGITS[10][5] = {
	{ "111", "101", "101", "101", "111" },
	{ "010", "110", "010", "010", "111" },
	{ "111", "001", "111", "100", "111" },
	{ "111", "001", "111", "001", "111" },
	{ "101", "101", "111", "001", "001" },
	{ "111", "100", "111", "001", "111" },
	{ "111", "100", "111", "101", "111" },
	{ "111", "001", "001", "001", "001" },
	{ "111", "101", "111", "101", "111" },
	{ "111", "101", "111", "001", "111" }
};

const char* HEART[11] = {
	"01100110",
	"11111111",
	"11111111",
	"11111111",
	"01111110",
	"00111100",
	"00011000",
	"00000000",
	"00000000",
	"00000000",
	"00000000"
};

void getBeijingTime(struct tm& outTime) {
	time_t now = time(NULL) + BEIJING_UTC_OFFSET_SECONDS;
	gmtime_r(&now, &outTime);
}

} // namespace

HeartClockPage::HeartClockPage()
	: PageBase("HeartClockPage") {
}

HeartClockPage::~HeartClockPage() {
}

void HeartClockPage::onEnter() {
	LOGD_TRACE("HeartClockPage: onEnter");
}

void HeartClockPage::onExit() {
	LOGD_TRACE("HeartClockPage: onExit");
}

bool HeartClockPage::onKeyEvent(int keyCode, int keyStatus) {
	if (keyStatus != 1) {
		return true;
	}

	if (keyCode == E_KEYCODE_MIDDLE_BUTTON || keyCode == E_KEYCODE_KNOB_BUTTON) {
		draw();
		return false;
	}

	return true;
}

void HeartClockPage::draw() {
	Surface surface(DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_BLACK);

	struct tm localTime;
	getBeijingTime(localTime);

	Color heartColor = (localTime.tm_sec % 2 == 0) ? COLOR_RED : COLOR_DIM_RED;
	drawHeart(surface, 2, 2, heartColor);

	int hour = localTime.tm_hour;
	int minute = localTime.tm_min;
	drawDigit(surface, 20, 3, hour / 10, COLOR_WHITE);
	drawDigit(surface, 27, 3, hour % 10, COLOR_WHITE);
	drawColon(surface, 34, 3, (localTime.tm_sec % 2 == 0) ? COLOR_BLUE : COLOR_BLACK);
	drawDigit(surface, 38, 3, minute / 10, COLOR_WHITE);
	drawDigit(surface, 45, 3, minute % 10, COLOR_WHITE);

	std::vector<uint8_t> data;
	surface.extractRGB(data);
	sendLedData(data);
}

void HeartClockPage::drawHeart(Surface& surface, int x, int y, const Color& color) {
	Painter& painter = Painter::getInstance();

	for (int row = 0; row < 7; ++row) {
		for (int col = 0; col < 8; ++col) {
			if (HEART[row][col] == '1') {
				painter.drawRect(surface, x + col * 2, y + row * 2, 2, 2, color, true);
			}
		}
	}
}

void HeartClockPage::drawTime(Surface& surface, int x, int y) {
	struct tm localTime;
	getBeijingTime(localTime);

	int hour = localTime.tm_hour;
	int minute = localTime.tm_min;

	drawDigit(surface, x, y, hour / 10, COLOR_WHITE);
	drawDigit(surface, x + 7, y, hour % 10, COLOR_WHITE);
	drawColon(surface, x + 14, y, (localTime.tm_sec % 2 == 0) ? COLOR_BLUE : COLOR_BLACK);
	drawDigit(surface, x + 18, y, minute / 10, COLOR_WHITE);
	drawDigit(surface, x + 25, y, minute % 10, COLOR_WHITE);
}

void HeartClockPage::drawDigit(Surface& surface, int x, int y, int digit, const Color& color) {
	if (digit < 0 || digit > 9) {
		return;
	}

	Painter& painter = Painter::getInstance();
	for (int row = 0; row < 5; ++row) {
		for (int col = 0; col < 3; ++col) {
			if (DIGITS[digit][row][col] == '1') {
				painter.drawRect(surface, x + col * 2, y + row * 2, 2, 2, color, true);
			}
		}
	}
}

void HeartClockPage::drawColon(Surface& surface, int x, int y, const Color& color) {
	Painter& painter = Painter::getInstance();
	painter.drawRect(surface, x, y + 2, 2, 2, color, true);
	painter.drawRect(surface, x, y + 6, 2, 2, color, true);
}
