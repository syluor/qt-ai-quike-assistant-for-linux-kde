#include <QApplication>
#include <QTextDocument>
#include <QLabel>
#include <QDebug>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QLabel label;
    label.setTextFormat(Qt::MarkdownText);
    label.setText("<style>h1 { font-size: 13px; font-weight: bold; }</style>\n\n# Header 1\n## Header 2\nSome *text* here.");
    qDebug() << "HTML from Markdown:" << label.text(); // just raw
    return 0;
}
