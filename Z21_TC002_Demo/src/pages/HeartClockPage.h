#ifndef PAGES_HEARTCLOCKPAGE_H_
#define PAGES_HEARTCLOCKPAGE_H_

#include "pages/PageBase.h"
#include "utils/Surface.h"

class HeartClockPage : public PageBase {
public:
	HeartClockPage();
	virtual ~HeartClockPage();

	virtual void draw() override;
	virtual void onEnter() override;
	virtual void onExit() override;
	virtual bool onKeyEvent(int keyCode, int keyStatus) override;

private:
	void drawHeart(Surface& surface, int x, int y, const Color& color);
	void drawTime(Surface& surface, int x, int y);
	void drawDigit(Surface& surface, int x, int y, int digit, const Color& color);
	void drawColon(Surface& surface, int x, int y, const Color& color);
};

#endif /* PAGES_HEARTCLOCKPAGE_H_ */
