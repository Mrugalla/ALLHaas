#include "Layout.h"

namespace gui
{
	BoundsF smallestBoundsIn(const LineF& line) noexcept
	{
		return { line.getStart(), line.getEnd() };
	}

	BoundsF maxQuadIn(const BoundsF& b) noexcept
	{
		const auto minDimen = std::min(b.getWidth(), b.getHeight());
		const auto x = b.getX() + .5f * (b.getWidth() - minDimen);
		const auto y = b.getY() + .5f * (b.getHeight() - minDimen);
		return { x, y, minDimen, minDimen };
	}

	BoundsF maxQuadIn(const Bounds& b) noexcept
	{
		return maxQuadIn(b.toFloat());
	}
}