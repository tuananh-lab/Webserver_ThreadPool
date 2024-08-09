#include <iostream>
#include <string>


// Abstract base class for Button
class Button {
public:
    virtual void click() = 0; // Pure virtual function
    virtual ~Button() = default;
};

// Abstract base class for ScrollBar
class ScrollBar {
public:
    virtual void scroll() = 0; // Pure virtual function
    virtual ~ScrollBar() = default;
};

// Concrete class for Light Button
class LightButton : public Button {
public:
    void click() override {
        std::cout << "Light Button Clicked" << std::endl;
    }
};

// Concrete class for Dark Button
class DarkButton : public Button {
public:
    void click() override {
        std::cout << "Dark Button Clicked" << std::endl;
    }
};

// Concrete class for Light ScrollBar
class LightScrollBar : public ScrollBar {
public:
    void scroll() override {
        std::cout << "Light ScrollBar Scrolling" << std::endl;
    }
};

// Concrete class for Dark ScrollBar
class DarkScrollBar : public ScrollBar {
public:
    void scroll() override {
        std::cout << "Dark ScrollBar Scrolling" << std::endl;
    }
};

// Abstract Factory interface
class WidgetFactory {
public:
    virtual ScrollBar* createScrollBar() = 0;
    virtual Button* createButton() = 0;
    virtual ~WidgetFactory() = default;
};

// Concrete Factory for Light Theme
class LightThemeWidgetFactory : public WidgetFactory {
public:
    ScrollBar* createScrollBar() override {
        std::cout << "Creating Light ScrollBar" << std::endl;
        return new LightScrollBar();
    }

    Button* createButton() override {
        std::cout << "Creating Light Button" << std::endl;
        return new LightButton();
    }
};

// Concrete Factory for Dark Theme
class DarkThemeWidgetFactory : public WidgetFactory {
public:
    ScrollBar* createScrollBar() override {
        std::cout << "Creating Dark ScrollBar" << std::endl;
        return new DarkScrollBar();
    }

    Button* createButton() override {
        std::cout << "Creating Dark Button" << std::endl;
        return new DarkButton();
    }
};

void showMenu() {
    std:: cout << "Choose Theme:\n";
    std:: cout << "1. Light Theme\n";
    std:: cout << "2. Dark Theme\n";
}

int main() {
    int themeChoice;
    WidgetFactory *factoryPtr = nullptr;
    showMenu();
    std:: cout << "Enter your choice: ";
    std:: cin >> themeChoice;

    switch (themeChoice) {
    case 1:
        factoryPtr = new LightThemeWidgetFactory();
        break;
    case 2:
        factoryPtr = new DarkThemeWidgetFactory();
        break;
    default:
        std::cout << "Invalid choice. Exiting...\n";
        return 1;
    } 

    std::cout << "\nChoose Widget Type:\n";
    std::cout << "1. ScrollBar\n";
    std::cout << "2. Button\n";
    std::cout << "Enter your choice: ";
    int widgetChoice;
    std::cin >> widgetChoice;

    switch (widgetChoice) {
    case 1: {
        ScrollBar* scrollbar = factoryPtr->createScrollBar();
        scrollbar->scroll();
        delete scrollbar;
        break;
    }
    case 2: {
        Button* button = factoryPtr->createButton();
        button->click();
        delete button;
        break;
    }
    default:
        std::cout << "Invalid choice. Exiting...\n";
        delete factoryPtr;
        return 1;
    }

    delete factoryPtr;
    return 0;
}
