#ifndef EXAMPLE_MAKEUP_
#define EXAMPLE_MAKEUP_

#include <string>

void detectFace(const std::string& image_name);
void mark(const std::string& image_name);

void detectSkin(const std::string& image_name);
void judgeFaceShape(const std::string& image_name);

void createShape();
void transform(const std::string& image_name);
void imageWarp();

void morphology(const std::string& image_name);

void applyLip(const std::string& image_name);
void applyBlush(const std::string& image_name);
void applyEyeShadow(const std::string& image_name);
void applyEyeLash(const std::string& image_name);
void applyBrow(const std::string& image_name);

#endif /* EXAMPLE_MAKEUP_ */