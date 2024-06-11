// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#include "ui/widgets/fields/custom_field_object.h"

#include "ui/text/text.h"
#include "ui/text/text_renderer.h"
#include "ui/text/text_utilities.h"
#include "ui/widgets/fields/input_field.h"
#include "styles/style_basic.h"
#include "styles/style_widgets.h"

namespace Ui {

CustomFieldObject::CustomFieldObject(
	not_null<InputField*> field,
	Factory factory,
	Fn<bool()> paused)
: _field(field)
, _factory(std::move(factory))
, _paused(std::move(paused))
, _now(crl::now()) {
}

CustomFieldObject::~CustomFieldObject() = default;

void *CustomFieldObject::qt_metacast(const char *iid) {
	if (QLatin1String(iid) == qobject_interface_iid<QTextObjectInterface*>()) {
		return static_cast<QTextObjectInterface*>(this);
	}
	return QObject::qt_metacast(iid);
}

QSizeF CustomFieldObject::intrinsicSize(
		QTextDocument *doc,
		int posInDocument,
		const QTextFormat &format) {
	const auto line = _field->_st.style.font->height;
	if (format.objectType() == InputField::kCollapsedQuoteFormat) {
		const auto &padding = _field->_st.style.blockquote.padding;
		const auto paddings = padding.left() + padding.right();
		const auto skip = 2 * doc->documentMargin();
		const auto height = Text::kQuoteCollapsedLines * line;
		return QSizeF(doc->pageSize().width() - paddings - skip, height);
	}
	const auto size = st::emojiSize * 1.;
	const auto width = size + st::emojiPadding * 2.;
	const auto height = std::max(line * 1., size);
	if (!_skip) {
		const auto emoji = Text::AdjustCustomEmojiSize(st::emojiSize);
		_skip = (st::emojiSize - emoji) / 2;
	}
	return { width, height };
}

void CustomFieldObject::drawObject(
		QPainter *painter,
		const QRectF &rect,
		QTextDocument *doc,
		int posInDocument,
		const QTextFormat &format) {
	if (format.objectType() == InputField::kCollapsedQuoteFormat) {
		const auto &st = _field->_st.style.blockquote;
		const auto left = 0;
		const auto top = 0;
		const auto id = format.property(InputField::kQuoteId).toInt();
		if (const auto i = _quotes.find(id); i != end(_quotes)) {
			i->second.string.draw(*painter, {
				.position = QPoint(left + rect.x(), top + rect.y()),
				.outerWidth = int(base::SafeRound(doc->pageSize().width())),
				.availableWidth = int(std::floor(rect.width())),

				.palette = nullptr,
				.spoiler = Text::DefaultSpoilerCache(),
				.now = _now,
				.paused = _paused(),

				.elisionLines = Text::kQuoteCollapsedLines,
			});
		}
		return;
	}
	const auto id = format.property(InputField::kCustomEmojiId).toULongLong();
	if (!id) {
		return;
	}
	auto i = _emoji.find(id);
	if (i == end(_emoji)) {
		const auto link = format.property(InputField::kCustomEmojiLink);
		const auto data = InputField::CustomEmojiEntityData(link.toString());
		if (auto emoji = _factory(data)) {
			i = _emoji.emplace(id, std::move(emoji)).first;
		}
	}
	if (i == end(_emoji)) {
		return;
	}
	i->second->paint(*painter, {
		.textColor = format.foreground().color(),
		.now = _now,
		.position = QPoint(
			int(base::SafeRound(rect.x())) + st::emojiPadding + _skip,
			int(base::SafeRound(rect.y())) + _skip),
		.paused = _paused && _paused(),
	});
}

void CustomFieldObject::clear() {
	_emoji.clear();
	_quotes.clear();
}

void CustomFieldObject::setCollapsedText(int quoteId, TextWithTags text) {
	auto &quote = _quotes[quoteId];
	quote.string.setMarkedText(_field->_st.style, {
		text.text,
		TextUtilities::ConvertTextTagsToEntities(text.tags),
	});
	quote.text = std::move(text);
}

TextWithTags CustomFieldObject::collapsedText(int quoteId) const {
	if (const auto i = _quotes.find(quoteId); i != end(_quotes)) {
		return i->second.text;
	}
	return {};
}

void CustomFieldObject::setNow(crl::time now) {
	_now = now;
}

} // namespace Ui
