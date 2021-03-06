#pragma once

class Memory;
class Window;

class Gpu {
public:
	Gpu(Memory &MemRef, Window &WindowRef) : clk(0), frames(0), enabled(true), MEM(MemRef), WINDOW(WindowRef) {}

	void update(unsigned tclock);
	unsigned clk;
	unsigned long frames;
	bool enabled;

private:
	Memory &MEM;
	Window &WINDOW;

	void set_status(uint8_t mode);
};