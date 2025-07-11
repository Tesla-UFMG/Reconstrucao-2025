#ifndef INTERFACE_IPLAYABLE_HPP
#define INTERFACE_IPLAYABLE_HPP

class IPlayable {
public:
    virtual ~IPlayable() = default;

    virtual void play() = 0;

    virtual void pause() = 0;

    virtual void seek(double position) = 0;

    virtual bool isPlaying() const = 0;

    virtual bool isLoaded() const = 0;

    virtual double getCurrentTime() const = 0;

    virtual double getDuration() const = 0;

    virtual const char* getTitle() const = 0;

    virtual float getStepSize() const = 0;

    virtual void setStepSize(float size) = 0;
};

#endif 