/* 
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdafx.h"
#include "Resource.h"

#include "ChatFilterPage.h"
#include "../client/version.h"
#include "WinUtil.h"
#include "ChatFilterDlg.h"

PropPage::TextItem ChatFilterPage::texts[] = {
	{ IDC_IGNORE_ADD, ResourceManager::ADD },
	{ IDC_IGNORE_REMOVE, ResourceManager::REMOVE },
	{ IDC_IGNORE_EDIT, ResourceManager::SETTINGS_CHANGE },
	{ IDC_IGNORE_CLEAR, ResourceManager::CLEAR },
	{ IDC_MISC_IGNORE, ResourceManager::SETTINGS_CHATFILTER },
	{ 0, ResourceManager::SETTINGS_AUTO_AWAY }
};

LRESULT ChatFilterPage::onInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	PropPage::translate((HWND)(*this), texts);

	CRect rc;
	
	ChatFilterListCtrl.Attach(GetDlgItem(IDC_IGNORELIST));
	ChatFilterListCtrl.GetClientRect(rc);
	ChatFilterListCtrl.InsertColumn(0, CTSTRING(IGNORE_NICK_MATCH), LVCFMT_LEFT, rc.Width() / 3, 1);
	ChatFilterListCtrl.InsertColumn(1, CTSTRING(IGNORE_TEXT_MATCH), LVCFMT_LEFT, rc.Width() / 3, 1);
	ChatFilterListCtrl.InsertColumn(2, CTSTRING(MAIN_CHAT), LVCFMT_LEFT, rc.Width() / 3 / 2, 1);
	ChatFilterListCtrl.InsertColumn(3, CTSTRING(PRIVATE_CHAT), LVCFMT_LEFT, rc.Width() / 3 / 2, 1);
	ChatFilterListCtrl.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	ChatFilterItems = IgnoreManager::getInstance()->getIgnoreList();
	for(auto& def: ChatFilterItems) {
		int p = ChatFilterListCtrl.insert(ChatFilterListCtrl.GetItemCount(), Text::toT(def.getNickPattern()));
		ChatFilterListCtrl.SetItemText(p, 1, Text::toT(def.getTextPattern()).c_str());
		ChatFilterListCtrl.SetItemText(p, 2, def.matchMainchat ? CTSTRING(YES) : CTSTRING(NO));
		ChatFilterListCtrl.SetItemText(p, 3, def.matchPM ? CTSTRING(YES) : CTSTRING(NO));
	}

	return TRUE;
}

LRESULT ChatFilterPage::onItemchangedDirectories(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
	bool hasSel = ChatFilterListCtrl.GetSelectedCount() > 0;
	::EnableWindow(GetDlgItem(IDC_IGNORE_EDIT), hasSel);
	::EnableWindow(GetDlgItem(IDC_IGNORE_REMOVE), hasSel);
	return 0;
}

LRESULT ChatFilterPage::onChatFilterAdd(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	ChatFilterDlg dlg;
	if(dlg.DoModal() == IDOK) {
		auto k = addChatFilter(dlg.nick, dlg.text, dlg.nickMethod, dlg.textMethod, dlg.MC, dlg.PM);
		if(k) {
			int p = ChatFilterListCtrl.insert(ChatFilterListCtrl.GetItemCount(), Text::toT(dlg.nick));
			ChatFilterListCtrl.SetItemText(p, 1, Text::toT(dlg.text).c_str());
			ChatFilterListCtrl.SetItemText(p, 2, dlg.MC ? CTSTRING(YES) : CTSTRING(NO));
			ChatFilterListCtrl.SetItemText(p, 3, dlg.PM ? CTSTRING(YES) : CTSTRING(NO));
		} else {
			WinUtil::showMessageBox(TSTRING(ALREADY_IGNORED), MB_OK);
		}
	}
	return 0;
}
LRESULT ChatFilterPage::onChatFilterEdit(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	int sel = ChatFilterListCtrl.GetSelectedIndex();
	if (sel == -1)
		return 0;

	auto olditem = getChatFilter(sel);
	ChatFilterDlg dlg(olditem.getNickPattern(), olditem.getTextPattern(), olditem.getNickMethod(), olditem.getTextMethod(), olditem.matchMainchat, olditem.matchPM);
	if (dlg.DoModal() == IDOK) {
		auto newitem = IgnoreItem(dlg.nick, dlg.text, dlg.nickMethod, dlg.textMethod, dlg.MC, dlg.PM);
		updateChatFilter(newitem, sel);
		ChatFilterListCtrl.SetItemText(sel, 0, Text::toT(dlg.nick).c_str());
		ChatFilterListCtrl.SetItemText(sel, 1, Text::toT(dlg.text).c_str());
		ChatFilterListCtrl.SetItemText(sel, 2, dlg.MC ? CTSTRING(YES) : CTSTRING(NO));
		ChatFilterListCtrl.SetItemText(sel, 3, dlg.PM ? CTSTRING(YES) : CTSTRING(NO));
	}

	return 0;
}

LRESULT ChatFilterPage::onChatFilterRemove(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	int i = -1;
	
	TCHAR buf[256];

	while((i = ChatFilterListCtrl.GetNextItem(-1, LVNI_SELECTED)) != -1) {
		ChatFilterListCtrl.GetItemText(i, 0, buf, 256);

		removeChatFilter(i);
		ChatFilterListCtrl.DeleteItem(i);
	}

	return 0;
}

LRESULT ChatFilterPage::onChatFilterClear(WORD /* wNotifyCode */, WORD /*wID*/, HWND /* hWndCtl */, BOOL& /* bHandled */) {
	ChatFilterListCtrl.DeleteAllItems();
	clearChatFilters();
	return 0;
}

LRESULT ChatFilterPage::onDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/) {
	NMITEMACTIVATE* item = (NMITEMACTIVATE*)pnmh;

	if (item->iItem >= 0) {
		PostMessage(WM_COMMAND, IDC_IGNORE_EDIT, 0);
	}
	else if (item->iItem == -1) {
		PostMessage(WM_COMMAND, IDC_IGNORE_ADD, 0);
	}

	return 0;
}

bool ChatFilterPage::addChatFilter(const string& aNick, const string& aText, StringMatch::Method aNickMethod, StringMatch::Method aTextMethod, bool aMainChat, bool aPM) {
	auto i = find_if(ChatFilterItems.begin(), ChatFilterItems.end(), [aNick, aText](const IgnoreItem& s) {
		return ((s.getNickPattern().empty() || compare(s.getNickPattern(), aNick) == 0) && (s.getTextPattern().empty() || compare(s.getTextPattern(), aText) == 0));
	});
	if (i == ChatFilterItems.end()){
		ChatFilterItems.push_back(IgnoreItem(aNick, aText, aNickMethod, aTextMethod, aMainChat, aPM));
		return true;
	}
	return false;
}

void ChatFilterPage::removeChatFilter(int pos) {
	ChatFilterItems.erase(ChatFilterItems.begin() + pos);
}

void ChatFilterPage::write() {
	IgnoreManager::getInstance()->replaceList(ChatFilterItems);
}