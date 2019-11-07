#include "Window.h"
#include "Platform.h"
#include "Font.h"
#include "Defines.h"
#include "System.h"
#include "Input.h"
#include "Desktop.h"
#include "WindowManager.h"
#include "Generated/Sprites.h"

Window WindowManager::windows[maxWindows];
uint8_t WindowManager::drawOrder[maxWindows];
uint8_t WindowManager::currentElementIndex;

constexpr int windowBarHeight = 8;
constexpr int closeButtonSize = windowBarHeight - 4;
constexpr int closeButtonX = 5;

Element Window::GetCurrentElement() const
{
	return Element(GetHandle(), WindowManager::currentElementIndex);
}

void Window::Draw()
{
	if (type == WindowType::FullWindow)
	{
		Platform::FillRect(x, y, w, h, WHITE);
		Platform::DrawRect(x, y, w, h, BLACK);

		// Drop shadow
		Platform::DrawFastHLine(x + 1, y + h, w, BLACK);
		Platform::DrawFastVLine(x + w, y + 1, h, BLACK);

		Platform::DrawFastHLine(x, y + windowBarHeight, w, BLACK);

		for (int j = 2; j < windowBarHeight - 1; j += 2)
		{
			Platform::DrawFastHLine(x + 2, y + j, w - 4, BLACK);
		}

		// Title
		int titleLength = title.Length() * Font::glyphWidth;
		constexpr int titlePadding = 4;
		Platform::FillRect(x + w / 2 - titleLength / 2 - titlePadding / 2, y + 1, titleLength + titlePadding, windowBarHeight - 2, WHITE);
		Font::DrawString(title, x + w / 2 - titleLength / 2, y + 2, BLACK);

		// Close button
		Platform::FillRect(x + closeButtonX - 1, y + 1, closeButtonSize + 3, windowBarHeight - 1, WHITE);
		Platform::DrawRect(x + closeButtonX, y + 2, closeButtonSize + 1, closeButtonSize + 1, BLACK);

		if (System::state.currentState == System::State::ClickingCloseWindowButton && System::state.stateElement.window == GetHandle())
		{
			Platform::DrawPixel(x + closeButtonX + 2, y + 4, BLACK);
		}
	}
	else if (type == WindowType::DialogBox)
	{
		Platform::FillRect(x, y, w, h, WHITE);
		Platform::DrawRect(x, y, w, h, BLACK);
		Platform::DrawRect(x + 2, y + 2, w - 4, h - 4, BLACK);
	}

	HandleEvent(SystemEvent::Repaint);
}

bool Window::IsMouseOverCloseButton() const
{
	return mouse.y > y && mouse.y < y + windowBarHeight && mouse.x > x + closeButtonX && mouse.x <= x + closeButtonX + closeButtonSize;
}

void Window::HandleEvent(SystemEvent eventType)
{
	if (type == WindowType::FullWindow)
	{
		if (eventType == SystemEvent::MouseDown)
		{
			if (mouse.y < y + windowBarHeight)
			{
				if (IsMouseOverCloseButton())
				{
					System::EnterState(System::State::ClickingCloseWindowButton, GetHandle());
					System::MarkScreenDirty();
				}
				else
				{
					System::EnterState(System::State::DraggingWindow, GetHandle());
				}
			}
		}
	}

	if (System::state.currentState == System::State::DraggingWindow)
	{
		if (eventType == SystemEvent::MouseMove)
		{
			if (y + mouse.deltaY < Desktop::menuBarHeight)
			{
				mouse.deltaY = Desktop::menuBarHeight - y;
			}

			x += mouse.deltaX;
			y += mouse.deltaY;
			System::MarkScreenDirty();
		}
		else if (eventType == SystemEvent::MouseUp)
		{
			System::ExitState(System::State::DraggingWindow);
		}
	}
	else if (System::state.currentState == System::State::ClickingCloseWindowButton)
	{
		if (eventType == SystemEvent::MouseUp)
		{
			System::ExitState(System::State::ClickingCloseWindowButton);
			WindowManager::Destroy(this);
		}
	}

	WindowManager::currentElementIndex = 0;
	if (handler)
	{
		handler(this, eventType);
	}
}

bool Window::Item(const uint8_t* icon, const xString& label, int16_t x, int16_t y)
{
	WindowManager::currentElementIndex++;

	bool result = false;
	constexpr int iconWidth = 9;
	constexpr int iconHeight = 12;
	constexpr int labelPadding = 2;
	int labelWidth = label.Length() * Font::glyphWidth + labelPadding;
	int labelHeight = Font::glyphHeight + labelPadding;
	int labelX = x + iconWidth / 2 - labelWidth / 2;
	int labelY = y + iconHeight;

	bool mouseOverItem = mouse.x >= x && mouse.y >= y && mouse.x < x + iconWidth && mouse.y < y + iconHeight;
	mouseOverItem |= mouse.x >= labelX && mouse.y >= labelY && mouse.x < labelX + labelWidth && mouse.y < labelY + labelHeight;
	bool isSelected = System::state.selectedElement == GetCurrentElement();

	switch (System::state.currentEvent)
	{
	case SystemEvent::Repaint:
		Platform::DrawSprite(x, y, icon, 0);
		Platform::FillRect(labelX, labelY, labelWidth, labelHeight, isSelected ? BLACK : WHITE);
		Font::DrawString(label, labelX + 1, labelY + 1, isSelected ? WHITE : BLACK);
		break;
	case SystemEvent::MouseDown:
		if (mouseOverItem)
		{
			if (isSelected)
			{
				result = true;
			}
			System::state.selectedElement = GetCurrentElement();
			System::MarkScreenDirty();
		}
		break;
	}

	return result;
}


bool Window::Button(xString label, int16_t buttonX, int16_t buttonY)
{
	WindowManager::currentElementIndex++;

	bool result = false;
	buttonX += x;
	buttonY += y + windowBarHeight;

	constexpr int buttonPadding = 4;
	int labelLength = label.Length() * Font::glyphWidth;
	int buttonWidth = labelLength + buttonPadding;
	int buttonHeight = Font::glyphHeight + buttonPadding;

	bool mouseOver = mouse.x >= buttonX && mouse.y >= buttonY && mouse.x < buttonX + buttonWidth && mouse.y < buttonY + buttonHeight;
	bool isClicked = System::state.currentState == System::State::ClickingButton && System::state.stateElement == GetCurrentElement();
	bool showClicked = isClicked && mouseOver;

	switch(System::state.currentEvent)
	{
	case SystemEvent::Repaint:
		Platform::FillRect(buttonX, buttonY, buttonWidth, buttonHeight, showClicked ? BLACK : WHITE);
		Font::DrawString(label, buttonX + buttonPadding / 2, buttonY + buttonPadding / 2, showClicked ? WHITE : BLACK);
		Platform::DrawFastHLine(buttonX + 1, buttonY, buttonWidth - 2, BLACK);
		Platform::DrawFastHLine(buttonX + 1, buttonY + buttonHeight - 1, buttonWidth - 2, BLACK);
		Platform::DrawFastVLine(buttonX, buttonY + 1, buttonHeight - 2, BLACK);
		Platform::DrawFastVLine(buttonX + buttonWidth - 1, buttonY + 1, buttonHeight - 2, BLACK);
		break;
	case SystemEvent::MouseDown:
		if (mouseOver)
		{
			System::EnterState(System::State::ClickingButton, GetCurrentElement());
			System::MarkScreenDirty();
		}
		break;
	case SystemEvent::MouseMove:
		if (isClicked)
		{
			System::MarkScreenDirty();
		}
		break;
	case SystemEvent::MouseUp:
		if (isClicked)
		{
			if (mouseOver)
			{
				result = true;
			}
			System::ExitState(System::State::ClickingButton);
			System::MarkScreenDirty();
		}
		break;
	}

	return result;
}

void Window::Slider(int16_t sliderX, int16_t sliderY, uint8_t sliderWidth, uint8_t& current)
{
	WindowManager::currentElementIndex++;

	constexpr int sliderHeight = 5;
	constexpr int widgetSize = 8;
	sliderX += x;
	sliderY += y;

	int innerLength = sliderWidth - 4;
	int innerX = sliderX + 2;

	int widgetPos = innerX + (current * innerLength) / 255;
	int widgetDrawX = widgetPos - widgetSize / 2;
	int widgetDrawY = sliderY + sliderHeight / 2 - widgetSize / 2;
	
	switch (System::state.currentEvent)
	{
	case SystemEvent::Repaint:
		Platform::FillRect(sliderX, sliderY, sliderWidth, sliderHeight, WHITE);
		Platform::DrawRect(sliderX, sliderY, sliderWidth, sliderHeight, BLACK);
		Platform::DrawFastHLine(sliderX + 2, sliderY + 2, sliderWidth - 4, BLACK);

		Platform::FillRect(widgetDrawX, widgetDrawY, widgetSize, widgetSize, WHITE);
		Platform::DrawRect(widgetDrawX, widgetDrawY, widgetSize, widgetSize, BLACK);
		break;
	case SystemEvent::MouseDown:
		if (mouse.x >= widgetDrawX && mouse.y >= widgetDrawY && mouse.x < widgetDrawX + widgetSize && mouse.y < widgetDrawY + widgetSize)
		{
			System::EnterState(System::State::DraggingSlider, GetCurrentElement());
		}
		break;
	case SystemEvent::MouseMove:
		if (System::state.currentState == System::State::DraggingSlider && System::state.stateElement == GetCurrentElement())
		{
			int newValue = ((mouse.x - innerX) * 255) / innerLength;
			if (newValue < 0)
				newValue = 0;
			else if (newValue > 255)
				newValue = 255;
			current = newValue;
			System::MarkScreenDirty();
		}
		break;
	case SystemEvent::MouseUp:
		if (System::state.currentState == System::State::DraggingSlider && System::state.stateElement == GetCurrentElement())
		{
			System::ExitState(System::State::DraggingSlider);
		}
		break;
	}
}

void Window::Label(const xString& label, int16_t labelX, int16_t labelY)
{
	labelX += x;
	labelY += y;

	if (System::state.currentEvent == SystemEvent::Repaint)
	{
		Font::DrawString(label, labelX, labelY, BLACK);
	}
}

void Window::VerticalScrollBar(uint16_t& current, uint16_t max)
{
	WindowManager::currentElementIndex++;

	if (current > max)
		current = max;

	constexpr int scrollBarWidth = 10;
	constexpr int scrollWidgetSize = 9;
	int scrollBarX = x + w - scrollBarWidth;
	int scrollBarY1 = y + 9;
	int scrollBarLength = h - 10;
	int scrollBarY2 = y + h - 1;

	int innerLength = scrollBarLength - 22 - scrollWidgetSize;
	int innerY = scrollBarY1 + 11;
	int widgetPos = max > 0 ? innerY + (current * innerLength) / max : 0;

	switch (System::state.currentEvent)
	{
	case SystemEvent::Repaint:
		for (int j = 0; j < scrollBarLength; j += 2)
		{
			uint8_t offset = 0;
			for (int i = 0; i < scrollBarWidth; i += 2)
			{
				Platform::DrawPixel(scrollBarX + i, scrollBarY1 + j + offset, BLACK);
				offset = !offset;
			}
		}
		Platform::DrawFastVLine(scrollBarX - 1, scrollBarY1, scrollBarLength, BLACK);
		Platform::DrawSprite(scrollBarX, scrollBarY1, upIcon, 0);
		Platform::DrawSprite(scrollBarX, scrollBarY2 - 11, downIcon, 0);

		if (max > 0)
		{
			Platform::FillRect(scrollBarX, widgetPos, scrollWidgetSize, scrollWidgetSize, WHITE);
			Platform::DrawRect(scrollBarX, widgetPos, scrollWidgetSize, scrollWidgetSize, BLACK);
		}
		break;
	case SystemEvent::MouseDown:
		if (max > 0 && mouse.x >= scrollBarX && mouse.x < scrollBarX + scrollBarWidth && mouse.y >= scrollBarY1 && mouse.y < scrollBarY2)
		{
			if (mouse.y < scrollBarY1 + 11)
			{	// Up arrow
				if (current > 0)
				{
					current--;
					System::MarkScreenDirty();
				}
			}
			else if (mouse.y > scrollBarY2 - 11)
			{	// Down arrow
				if (current < max)
				{
					current++;
					System::MarkScreenDirty();
				}
			}
			else if(mouse.y >= widgetPos && mouse.y < widgetPos + scrollWidgetSize)
			{	// Widget
				System::EnterState(System::State::ScrollingWindow, GetCurrentElement());
			}
			else
			{	// Bar
				int newWidgetPos = mouse.y - scrollWidgetSize / 2;
				if (newWidgetPos < innerY)
					newWidgetPos = innerY;
				if (newWidgetPos > innerY + innerLength)
					newWidgetPos = innerY + innerLength;

				current = ((newWidgetPos - innerY) * max) / innerLength;
				System::MarkScreenDirty();
			}
		}
		break;
	case SystemEvent::MouseMove:
		if (System::state.currentState == System::State::ScrollingWindow && System::state.stateElement == GetCurrentElement())
		{
			int newWidgetPos = mouse.y - scrollWidgetSize / 2;
			if (newWidgetPos < innerY)
				newWidgetPos = innerY;
			if (newWidgetPos > innerY + innerLength)
				newWidgetPos = innerY + innerLength;
			//mouse.deltaY = newWidgetPos - widgetPos;

			current = ((newWidgetPos - innerY) * max) / innerLength;
			System::MarkScreenDirty();
			break;
		}
	case SystemEvent::MouseUp:
		if (System::state.currentState == System::State::ScrollingWindow && System::state.stateElement == GetCurrentElement())
		{
			System::ExitState(System::State::ScrollingWindow);
		}
		break;
	}
}

void Window::OpenWithAnimation(uint8_t fromX, uint8_t fromY)
{
	System::EnterState(System::State::OpeningWindowAnimation, Element(GetHandle()));
	originX = fromX;
	originY = fromY;
}

WindowHandle Window::GetHandle() const
{
	for (int n = 0; n < WindowManager::maxWindows; n++)
	{
		if (this == &WindowManager::windows[n])
			return n;
	}

	return WindowManager::invalidWindowHandle;
}
