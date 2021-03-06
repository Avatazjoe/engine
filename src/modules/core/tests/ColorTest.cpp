/**
 * @file
 */

#include "AbstractTest.h"
#include "core/Color.h"
#include <SDL_endian.h>

namespace core {

class ColorTest: public AbstractTest {
};

TEST_F(ColorTest, testRGBA) {
	core::RGBA color;
	color.rgba = SDL_SwapLE32(0xff6699fe);
	EXPECT_EQ(0xfe, color.r);
	EXPECT_EQ(0x99, color.g);
	EXPECT_EQ(0x66, color.b);
	EXPECT_EQ(0xff, color.a);

	const glm::vec4 fcolor = core::Color::fromRGBA(color.rgba);
	EXPECT_FLOAT_EQ(color.r / core::Color::magnitudef, fcolor.r);
	EXPECT_FLOAT_EQ(color.g / core::Color::magnitudef, fcolor.g);
	EXPECT_FLOAT_EQ(color.b / core::Color::magnitudef, fcolor.b);
	EXPECT_FLOAT_EQ(color.a / core::Color::magnitudef, fcolor.a);
	EXPECT_FLOAT_EQ(1.0f, fcolor.a);

	core::RGBA convertedBack;
	convertedBack.rgba = core::Color::getRGBA(fcolor);
	EXPECT_EQ(0xfe, convertedBack.r);
	EXPECT_EQ(0x99, convertedBack.g);
	EXPECT_EQ(0x66, convertedBack.b);
	EXPECT_EQ(0xff, convertedBack.a);
}

TEST_F(ColorTest, testHex) {
	EXPECT_EQ(glm::vec4(1.0f), core::Color::fromHex("#ffffff"));
	EXPECT_EQ(glm::vec4(1.0f), core::Color::fromHex("0xffffff"));
	EXPECT_EQ(glm::vec4(1.0f), core::Color::fromHex("0xffffffff"));
	EXPECT_EQ(glm::vec4(0.0f), core::Color::fromHex("0x00000000"));
	EXPECT_EQ(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), core::Color::fromHex("0xff0000ff"));
	EXPECT_EQ(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), core::Color::fromHex("#ff0000ff"));
}

}
