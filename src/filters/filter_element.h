/** @file filter_element.h Functions related to the basic elements for filters. */

#ifndef FILTER_ELEMENT_H
#define FILTER_ELEMENT_H

#include "../string_func.h"
#include "../strings_func.h"
#include "../gfx_type.h"
#include "filter_enums.h"

#include "table/strings.h"

struct FilterElement {
	private:
	uint32 element;     ///< Unique ID of the element (index on pool, most of the times).
	ItemStatus state;   ///< State of the filter element (neutral, positive, negative).

	public:
	FilterElement(uint32 new_element) : element(new_element), state(IS_NEUTRAL) {}

	FilterElement(uint32 new_element, ItemStatus it_state) : element(new_element), state(it_state) {}

	FilterElement(const FilterElement &copied) { this->Copy(copied); }

	void Copy(const FilterElement &copied)
	{
		this->element = copied.element;
		this->state = copied.state;
	}

	/**
	 * Reset the status of an element.
	 */
	void Reset() { this->state = IS_NEUTRAL; }

	uint GetElement() const { return this->element; }
	ItemStatus GetState() const { return this->state; }
	void SetState(ItemStatus new_state) { this->state = new_state; }

	bool IsActive()   const { return this->state >  IS_NEUTRAL; }
	bool IsPositive() const { return this->state == IS_POSITIVE; }
	bool IsNegative() const { return this->state == IS_NEGATIVE; }

	/**
	 * Toggle the active bit: neutral -> active -> neutral.
	 * @note Used when an element can choose between do nothing or do something, but not a third action.
	 * @pre State is neutral or positive.
	 */
	void ToggleActive() {
		switch (this->state) {
			case IS_NEUTRAL:
				this->state = IS_POSITIVE;
				return;
			case IS_POSITIVE:
				this->state = IS_NEUTRAL;
				return;
			default:
				NOT_REACHED();
		}
	}

	/**
	 * Rotate an element: neutral -> active and positive -> active and negative -> neutral.
	 */
	void Rotate()
	{
		this->state++;
		if (this->state == IS_END) this->state = IS_NEUTRAL;
	}

	/**
	 * Set a marker for an element of a filter list.
	 * @return A blank space if neutral, "v" active and positive, "x" active and negative
	 */
	StringID GetMark() const
	{
		switch (this->state) {
			case IS_NEUTRAL:
				SetDParamStr(0, " ");
				break;
			case IS_POSITIVE:
				SetDParamStr(0, "v");
				break;
			case IS_NEGATIVE:
				SetDParamStr(0, "x");
				break;
			default:
				NOT_REACHED();
		}

		return STR_BLACK_RAW_STRING;
	}

	/**
	 * Set the color of the string for an element of a filter list.
	 * @return Black for neutral, green active and positive, red active and negative.
	 */
	TextColour GetColourString() const
	{
		switch (this->state) {
			case IS_NEUTRAL:
				return TC_BLACK;
			case IS_POSITIVE:
				return TC_GREEN;
			case IS_NEGATIVE:
				return TC_RED;
			default:
				NOT_REACHED();
		}
	}

	/**
	 * Set the frame flags for an element.
	 * @return Default if neutral, lowered if active; lowered and darkened if active and negative.
	 */
	FrameFlags GetFrameFlags() const
	{
		switch (this->state) {
			case IS_NEUTRAL:
				return FR_NONE;
			case IS_POSITIVE:
				return FR_LOWERED;
			case IS_NEGATIVE:
				return FR_LOWERED | FR_DARKENED;
			default:
				NOT_REACHED();
		}
	}

	/**
	 * Comparison between elements to filter: true if same ID, false if different ID (regardless of state...).
	 */
	inline bool operator != (const FilterElement &other) const
	{
		return other.element != this->element;
	}
};

#endif /* FILTER_ELEMENT_H */