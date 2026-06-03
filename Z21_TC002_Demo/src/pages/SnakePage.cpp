#include "pages/SnakePage.h"
#include "utils/Painter.h"
#include "managers/KeyManager.h"
#include "base/base.h"
#include <cstdlib>
#include <ctime>

namespace {

#define DISPLAY_WIDTH  52
#define DISPLAY_HEIGHT 16

static const Color COLOR_BLACK(0, 0, 0);
static const Color COLOR_HEAD(120, 255, 80);
static const Color COLOR_BODY(0, 180, 45);
static const Color COLOR_FOOD(255, 20, 30);
static const Color COLOR_READY(30, 80, 180);
static const Color COLOR_PAUSED(180, 180, 0);
static const Color COLOR_GAME_OVER(180, 0, 0);

const int SPEED_INTERVALS[SnakePage::SPEED_LEVEL_COUNT] = {
	500, 350, 250, 180, 130
};

} // namespace

SnakePage::SnakePage()
	: PageBase("SnakePage"),
	  mFood({0, 0}),
	  mDirection(DIR_RIGHT),
	  mState(STATE_READY),
	  mSpeedLevel(1) {
	srand(time(NULL));
	resetGame();
}

SnakePage::~SnakePage() {
}

void SnakePage::onEnter() {
	LOGD_TRACE("SnakePage: onEnter");
	draw();
}

void SnakePage::onExit() {
	LOGD_TRACE("SnakePage: onExit");
}

bool SnakePage::onKeyEvent(int keyCode, int keyStatus) {
	switch (keyCode) {
	case E_KEYCODE_CLOCKWISE:
		if (mState == STATE_READY || mState == STATE_GAME_OVER) {
			return true;
		}
		if (mState == STATE_PAUSED) {
			draw();
			return false;
		}
		turnRight();
		draw();
		return false;
	case E_KEYCODE_ANTI_CLOCKWISE:
		if (mState == STATE_READY || mState == STATE_GAME_OVER) {
			return true;
		}
		if (mState == STATE_PAUSED) {
			draw();
			return false;
		}
		turnLeft();
		draw();
		return false;
	default:
		break;
	}

	if (keyStatus != 1) {
		return true;
	}

	switch (keyCode) {
	case E_KEYCODE_LEFT_BUTTON:
		changeSpeed(-1);
		draw();
		return false;
	case E_KEYCODE_RIGHT_BUTTON:
		changeSpeed(1);
		draw();
		return false;
	case E_KEYCODE_MIDDLE_BUTTON:
		handleMiddleButton();
		draw();
		return false;
	case E_KEYCODE_KNOB_BUTTON:
		handleKnobButton();
		draw();
		return false;
	default:
		return true;
	}
}

void SnakePage::tick() {
	if (mState != STATE_RUNNING) {
		draw();
		return;
	}

	stepSnake();
	draw();
}

int SnakePage::getTickIntervalMs() const {
	return SPEED_INTERVALS[mSpeedLevel];
}

void SnakePage::resetGame() {
	mSnake.clear();
	mSnake.push_back({BOARD_WIDTH / 2, BOARD_HEIGHT / 2});
	mSnake.push_back({BOARD_WIDTH / 2 - 1, BOARD_HEIGHT / 2});
	mSnake.push_back({BOARD_WIDTH / 2 - 2, BOARD_HEIGHT / 2});
	mDirection = DIR_RIGHT;
	mState = STATE_READY;
	spawnFood();
}

void SnakePage::handleKnobButton() {
	if (mState == STATE_READY) {
		mState = STATE_RUNNING;
		return;
	}
	if (mState == STATE_RUNNING) {
		mState = STATE_GAME_OVER;
		return;
	}
	if (mState == STATE_PAUSED) {
		mState = STATE_GAME_OVER;
		return;
	}
	if (mState == STATE_GAME_OVER) {
		resetGame();
		mState = STATE_RUNNING;
	}
}

void SnakePage::handleMiddleButton() {
	if (mState == STATE_RUNNING) {
		mState = STATE_PAUSED;
		return;
	}
	if (mState == STATE_PAUSED) {
		mState = STATE_RUNNING;
		return;
	}
	if (mState == STATE_READY || mState == STATE_GAME_OVER) {
		handleKnobButton();
	}
}

void SnakePage::turnLeft() {
	if (mState != STATE_RUNNING) {
		return;
	}
	mDirection = static_cast<Direction>((mDirection + 3) % 4);
}

void SnakePage::turnRight() {
	if (mState != STATE_RUNNING) {
		return;
	}
	mDirection = static_cast<Direction>((mDirection + 1) % 4);
}

void SnakePage::changeSpeed(int delta) {
	mSpeedLevel += delta;
	if (mSpeedLevel < 0) {
		mSpeedLevel = 0;
	}
	if (mSpeedLevel >= SPEED_LEVEL_COUNT) {
		mSpeedLevel = SPEED_LEVEL_COUNT - 1;
	}
}

void SnakePage::spawnFood() {
	for (int attempt = 0; attempt < BOARD_WIDTH * BOARD_HEIGHT * 2; ++attempt) {
		int x = rand() % BOARD_WIDTH;
		int y = rand() % BOARD_HEIGHT;
		if (!isSnakeCell(x, y)) {
			mFood = {x, y};
			return;
		}
	}

	for (int y = 0; y < BOARD_HEIGHT; ++y) {
		for (int x = 0; x < BOARD_WIDTH; ++x) {
			if (!isSnakeCell(x, y)) {
				mFood = {x, y};
				return;
			}
		}
	}
}

void SnakePage::stepSnake() {
	Cell head = nextHead();

	bool willEat = (head.x == mFood.x && head.y == mFood.y);
	if (isSnakeBodyCell(head.x, head.y)) {
		mState = STATE_GAME_OVER;
		return;
	}

	mSnake.insert(mSnake.begin(), head);
	if (willEat) {
		spawnFood();
	} else {
		mSnake.pop_back();
	}
}

bool SnakePage::isSnakeCell(int x, int y) const {
	for (size_t i = 0; i < mSnake.size(); ++i) {
		if (mSnake[i].x == x && mSnake[i].y == y) {
			return true;
		}
	}
	return false;
}

bool SnakePage::isSnakeBodyCell(int x, int y) const {
	size_t limit = mSnake.size();
	if (limit > 0) {
		limit -= 1;
	}
	for (size_t i = 0; i < limit; ++i) {
		if (mSnake[i].x == x && mSnake[i].y == y) {
			return true;
		}
	}
	return false;
}

SnakePage::Cell SnakePage::nextHead() const {
	Cell head = mSnake.front();

	switch (mDirection) {
	case DIR_UP:
		head.y = (head.y - 1 + BOARD_HEIGHT) % BOARD_HEIGHT;
		break;
	case DIR_RIGHT:
		head.x = (head.x + 1) % BOARD_WIDTH;
		break;
	case DIR_DOWN:
		head.y = (head.y + 1) % BOARD_HEIGHT;
		break;
	case DIR_LEFT:
		head.x = (head.x - 1 + BOARD_WIDTH) % BOARD_WIDTH;
		break;
	}

	return head;
}

void SnakePage::draw() {
	Surface surface(DISPLAY_WIDTH, DISPLAY_HEIGHT, COLOR_BLACK);
	drawBoard(surface);
	std::vector<uint8_t> data;
	surface.extractRGB(data);
	sendLedData(data);
}

void SnakePage::drawCell(Surface& surface, int x, int y, const Color& color) {
	Painter::getInstance().drawRect(
		surface,
		x * CELL_SIZE,
		y * CELL_SIZE,
		CELL_SIZE,
		CELL_SIZE,
		color,
		true);
}

void SnakePage::drawBoard(Surface& surface) {
	if (mState == STATE_READY) {
		drawStatusText(surface, "START", COLOR_READY);
		return;
	}
	if (mState == STATE_PAUSED) {
		drawStatusText(surface, "PAUSE", COLOR_PAUSED);
		return;
	}
	if (mState == STATE_GAME_OVER) {
		drawStatusText(surface, "GAME OVER", COLOR_GAME_OVER);
		return;
	}

	drawCell(surface, mFood.x, mFood.y, COLOR_FOOD);

	for (int i = static_cast<int>(mSnake.size()) - 1; i >= 0; --i) {
		drawCell(surface, mSnake[i].x, mSnake[i].y, i == 0 ? COLOR_HEAD : COLOR_BODY);
	}
}

void SnakePage::drawStatusText(Surface& surface, const std::string& text, const Color& color) {
	Painter& painter = Painter::getInstance();
	int width = painter.getTextWidth(text, 0);
	int x = (DISPLAY_WIDTH - width) / 2;
	if (x < 0) {
		x = 0;
	}
	painter.drawText(surface, x, 10, text, color, 0);
}
