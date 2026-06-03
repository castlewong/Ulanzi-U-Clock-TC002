#ifndef PAGES_SNAKEPAGE_H_
#define PAGES_SNAKEPAGE_H_

#include "pages/PageBase.h"
#include "utils/Surface.h"
#include <string>
#include <vector>

class SnakePage : public PageBase {
public:
	SnakePage();
	virtual ~SnakePage();

	virtual void draw() override;
	virtual void onEnter() override;
	virtual void onExit() override;
	virtual bool onKeyEvent(int keyCode, int keyStatus) override;

	void tick();
	int getTickIntervalMs() const;

	static const int SPEED_LEVEL_COUNT = 5;

private:
	struct Cell {
		int x;
		int y;
	};

	enum Direction {
		DIR_UP,
		DIR_RIGHT,
		DIR_DOWN,
		DIR_LEFT
	};

	enum GameState {
		STATE_READY,
		STATE_RUNNING,
		STATE_PAUSED,
		STATE_GAME_OVER
	};

	static const int BOARD_WIDTH = 26;
	static const int BOARD_HEIGHT = 8;
	static const int CELL_SIZE = 2;

	std::vector<Cell> mSnake;
	Cell mFood;
	Direction mDirection;
	GameState mState;
	int mSpeedLevel;
	void resetGame();
	void handleKnobButton();
	void handleMiddleButton();
	void turnLeft();
	void turnRight();
	void changeSpeed(int delta);
	void spawnFood();
	void stepSnake();
	bool isSnakeCell(int x, int y) const;
	bool isSnakeBodyCell(int x, int y) const;
	Cell nextHead() const;
	void drawCell(Surface& surface, int x, int y, const Color& color);
	void drawBoard(Surface& surface);
	void drawStatusText(Surface& surface, const std::string& text, const Color& color);
};

#endif /* PAGES_SNAKEPAGE_H_ */
