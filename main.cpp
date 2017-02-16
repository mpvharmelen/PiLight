#include "thread.h"
#include "led-matrix.h"
#include "LedServer.h"

#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <algorithm>

using std::min;
using std::max;

// Base-class for a Thread that does something with a matrix.
class RGBMatrixManipulator : public Thread {
public:
  RGBMatrixManipulator(RGBMatrix *m) : running_(true), matrix_(m) {}
  virtual ~RGBMatrixManipulator() { running_ = false; }

  // Run() implementation needs to check running_ regularly.

protected:
  volatile bool running_;  // TODO: use mutex, but this is good enough for now.
  RGBMatrix *const matrix_;
};

// Pump pixels to screen. Needs to be high priority real-time because jitter
// here will make the PWM uneven.
class DisplayUpdater : public RGBMatrixManipulator {
public:
  DisplayUpdater(RGBMatrix *m) : RGBMatrixManipulator(m) {}

  void Run() {
    while (running_) {
      matrix_->UpdateScreen();
    }
  }
};

class SimpleSquare : public RGBMatrixManipulator {
public:
  SimpleSquare(RGBMatrix *m) : RGBMatrixManipulator(m) {}
  void Run() {
    const int width = matrix_->width();
    const int height = matrix_->height();
    // Diagonaly
    for (int x = 0; x < width; ++x) {
        matrix_->SetPixel(x, x, 255, 255, 255);
        matrix_->SetPixel(height -1 - x, x, 255, 0, 255);
    }
    for (int x = 0; x < width; ++x) {
      matrix_->SetPixel(x, 0, 255, 0, 0);
      matrix_->SetPixel(x, height - 1, 255, 255, 0);
    }
    for (int y = 0; y < height; ++y) {
      matrix_->SetPixel(0, y, 0, 0, 255);
      matrix_->SetPixel(width - 1, y, 0, 255, 0);
    }
  }
};

// -- The following are demo image generators.
int main(int argc, char *argv[]) {
    std::cout << "Starting the LED controller" << '\n';
    // Init server
    LedServer *serv = new LedServer(50007);
    GPIO io;
    if (!io.Init())
        return 1;

    RGBMatrix m(&io);
    RGBMatrixManipulator *image_gen = new SimpleSquare(&m);
    RGBMatrixManipulator *updater = new DisplayUpdater(&m);
    updater->Start(10);  // high priority
    serv->Start(5);
    image_gen->Start();
    // Things are set up. Just wait for <RETURN> to be pressed.
    printf("Press <RETURN> to exit and reset LEDs\n");
    getchar();
    std::cout << "Stopping server.." << '\n';
    // Stopping threads and wait for them to join.
    delete updater;
    delete image_gen;

    // Final thing before exit: clear screen and update once, so that
    // we don't have random pixels burn
    m.ClearScreen();
    m.UpdateScreen();

    return 0;
    }
