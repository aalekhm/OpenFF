///*
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <iostream>
#include "Engine/InputManager.h"
#include "Engine/UI/WWidgetManager.h"
#include "Engine/UI/WComponentFactory.h"
#include "Engine/GameEngine.h"

class YAGui : public GameEngine
{
	public:
		YAGui()
		{
			createWindow(1024, 1024, "YAGui Window!");
		}

		virtual void onCreate()
		{
			m_pWidgetManager = WWidgetManager::getInstance();
			{
				m_fUIFuncCallback = std::bind(	&YAGui::onUICallback,
												this,
												std::placeholders::_1,
												std::placeholders::_2,
												std::placeholders::_3,
												std::placeholders::_4);

				WWidgetManager::setCallback(&m_fUIFuncCallback);
				m_pWidgetManager->init(m_iWidth, m_iHeight);				
			}

			m_fMouseFuncCallback = std::bind(	&YAGui::onMouseCallback,
												this, 
												std::placeholders::_1, 
												std::placeholders::_2, 
												std::placeholders::_3, 
												std::placeholders::_4);

			m_fKeyboardFuncCallback = std::bind(	&YAGui::onKeyboardCallback,
													this, 
													std::placeholders::_1, 
													std::placeholders::_2,
													std::placeholders::_3, 
													std::placeholders::_4);

			m_fUnicodeCharFuncCallback = std::bind(	&YAGui::onUnicodeCharCallback,
													this,
													std::placeholders::_1);
			
			InputManager::get()->addMouseListener(&m_fMouseFuncCallback);
			InputManager::get()->addKeyboardListener(&m_fKeyboardFuncCallback);
			InputManager::get()->addUnicodeCharListener(&m_fUnicodeCharFuncCallback);
		}

		virtual void onFirstFrame()
		{
			glfwSetTime(0.0);
		}

		virtual void onUpdate(uint32_t iDeltaTimeMs, uint64_t lElapsedTime)
		{
			glViewport(0, 0, m_iWidth, m_iHeight);

			glClearColor(0.2, 0.2, 0.2, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);

			m_pWidgetManager->update(iDeltaTimeMs);
		}

		virtual void onPaint(Graphics* pGraphics)
		{
			
		}

		virtual void onDestroy()
		{
		}

		void onMouseCallback(int32_t iAction, int32_t iKey, double dXPos, double dYPos)
		{
			switch (iAction)
			{
				case GLFW_PRESS:
				{
					m_pWidgetManager->onMouseDown(iKey, dXPos, dYPos);
				}
				break;
				case GLFW_RELEASE:
				{
					m_pWidgetManager->onMouseUp(iKey, dXPos, dYPos);
				}
				break;
				case GLFW_REPEAT:
				{
					if(iKey == 0)	// Mouse Move
						m_pWidgetManager->onMouseHover(iKey, dXPos, dYPos);
					else
					if (iKey > 0)	// Left Mouse Move
						m_pWidgetManager->onMouseMove(iKey, dXPos, dYPos);
				}
				break;
			}
		}

		void onKeyboardCallback(int32_t iAction, int32_t iKeyCode, int32_t iScanCode, int32_t iModifierKeys)
		{
			//std::cout << "iKeyCode = " << iKeyCode << ", iScanCode = " << iScanCode << ", iModifierKeys = " << iModifierKeys << "\n";
			switch (iAction)
			{
				case GLFW_PRESS:
				{
					if (iKeyCode == GLFW_KEY_UP
						||
						iKeyCode == GLFW_KEY_DOWN
						||
						iKeyCode == GLFW_KEY_LEFT
						||
						iKeyCode == GLFW_KEY_RIGHT
						||
						iKeyCode == GLFW_KEY_HOME
						||
						iKeyCode == GLFW_KEY_END
						||
						iKeyCode == GLFW_KEY_BACKSPACE
						||
						iKeyCode == GLFW_KEY_DELETE
						||
						iKeyCode == GLFW_KEY_PAGE_UP
						||
						iKeyCode == GLFW_KEY_PAGE_DOWN
					) {
						m_pWidgetManager->keyPressed(iKeyCode, 0);
					}
				}
				break;
				case GLFW_RELEASE:
				{
					//m_pWidgetManager->keyReleased(iKeyCode, 0);
				}
				break;
			}
		}

		void onUnicodeCharCallback(uint32_t iUnicodeChar)
		{
			//std::cout << "iUnicodeChar = " << iUnicodeChar << "\n";
			m_pWidgetManager->keyPressed(0, iUnicodeChar);
		}

		L_RESULT CALLBACK onUICallback(H_WND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
				case WN__CREATE:
				{
					if (wParam == ROOT_WINDOW_ID)
					{
						//addUIComponents(hWnd);
					}
				}
				break;
				case WN__PAINT:
				{

				}
				break;
				case WN__MOVE:
				{
				}
				break;
				case WN__SIZE:
				{
				}
				break;
				case WM_CBN_SELENDOK:
				{
					int WINDOW_ID = wParam;
					switch(WINDOW_ID) 
					{
						case IDC_CB_FONT:
							WComboBox* pComboBox = (WComboBox*)hWnd;
							H_FONT hFont = CreateFontQ(pComboBox->getText(), 10, 96);
							if(hFont != NULL)
								SelectFontQ(hFont);
						break;
					}
				}
				break;
			}

			return WM__OKAY;
		}

		void addUIComponents(H_WND hParent)
		{
			unsigned int iYPos = 0;
			unsigned int iXPos = 20;

			//NEW_OBJECT("WStatic");
			WComponentFactory* factory = WComponentFactory::Get();
			H_WND hWnd = NULL;

			hWnd = 
			CreateComponentEx(	"WStatic", 
										"Static Text... !!!", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										40, 
										189, 
										25,
										hParent, 
										HMENU(1234), 
										(LPVOID)255);

			hWnd = 
			CreateComponentEx(	"WButton", 
								"Simple Button...", 
								WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, 
								iXPos,
								90, 
								189, 
								25,
								hParent, 
								HMENU(121), 
								(LPVOID)"Button");

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WSprite", 
											"Simple Sprite...", 
											WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, 
											iXPos + 150,
											iYPos, 
											100, 
											100,
											hParent, 
											NULL, 
											(LPVOID)"Exclamation");
			{
				hWnd = 
				CreateComponentEx(	"WSprite", 
												"Simple Sprite inside Sprite...", 
												WM_ALIGN_WRT_PARENT_TOPLEFT | WM_ANCHOR_TOPLEFT, 
												10,
												10, 
												50, 
												50,
												hWnd, 
												NULL, 
												(LPVOID)"Exclamation");
			}

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WCheckbox", 
										"CheckBox text", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										100, 
										25,
										hParent, 
										HMENU(1102), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WCanvas", 
										"WCanvas window", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										100, 
										100,
										hParent, 
										HMENU(1103), 
										NULL);
				{
					H_WND hBtn = 
					CreateComponentEx(	"WButton", 
												"Simple Button", 
												WM_ANCHOR_TOPLEFT, 
												10,
												10, 
												50, 
												25,
												hWnd, 
												HMENU(1291), 
												(LPVOID)"Button");
				}
			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTextField", 
										"_______I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										200, 
										23,
										hParent, 
										HMENU(1099), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTextField", 
										"sprintf(m_pText, \"%s%c%s\", leftHalfSubstr, iKey, rightHalfSubstr); g->SetClip(RectF(m_pParent->getLeft(), m_pParent->getTop(), m_pParent->getWidth(), m_pParent->getHeight()));", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										220, 
										23,
										hParent, 
										HMENU(98), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTextField", 
										"**sprintf(m_pText, \"%s%c%s\", leftHalfSubstr, iKey, rightHalfSubstr); g->SetClip(RectF(m_pParent->getLeft(), m_pParent->getTop(), m_pParent->getWidth(), m_pParent->getHeight()));", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										220, 
										23,
										hParent, 
										HMENU(98), 
										NULL);
			//WScrollbar* sb = new WScrollbar();
			//sb->create(hParent, 20, 150, 250, 15, 0, 120);
			//addComponent(sb);
			//
			//WScrollbar* sb1 = new WScrollbar();
			//sb1->create(hParent, 30, 170, 15, 180, 1, 1211);
			//addComponent(sb1);

			CCString sText = "New Delhi, July 26 (IANS) Hours after saying he did not wish to implicate Prime Minister Manmohan Singh or anyone else in the 2G spectrum allotment case, former telecom minister A. Raja Tuesday asked why the matter had not been referred to a ministerial panel and also wanted Home Minister P. Chidambaram to take the witness stand.\n\
		Main kisi ko phasana nahi chahta tha (I had no intention of framing anybody),' Raja's lawyer Sushil Kumar said on his behalf when the names of Manmohan Singh and Chidambaram cropped up in a special Central Bureau of Investigation (CBI) court.\n\
		I am just defending myself -- not accusing anything or anybody,' he said, a day after stroking a political storm by dragging the prime minister into the controversy. 'They (the media) cannot put words into my mouth. Ask them to report truthfully, or go out of this court,' he added.\n\
		But the home minister must come in the court from either of the sides and be a witness in the case. When all decisions were known to the home minister, he should appear as a witness in the case,' Kumar told the special court presided over by Judge O.P. Saini.\n\
		Just a few hours later, after recess, he stepped up his attack on the prime minister and wondered why a group of ministers (GoM) was not set up if any inconsistency was found on the way the spectrum allocation matter was handled.\n\
		A lawyer by traininRaja himself took over from his counsel at one point.\n\
		The prime minister is superior to me. He could have constituted a GoM. But he ignored a GoM. Is this a conspiracy?' Raja's counsel asked, wanting the then solicitor general and now attorney general Goolam. E. Vahanvati, too, in the witness box, while terming the judicial custody of his client since Feb 2 illegal.\n\
		The counsel was continuing with the arguments the previous day that as finance minister in 2008 Chidambaram had taken the decision to permit the promoters of two telecom firms to sell stakes with full knowledge of the prime minister.\n\
		While this was not denied subsequently by Chidambaram or present Communications Minister Kapil Sibal, both sought to say that the equity sale was by way of issuing fresh shares and not divestment by promoters, permitted under the policy that existed then.\n\
		The Congress even launched a counter-attack Tuesday and said Raja had also dragged former prime minister Atal Bihari Vajpayee's name in the case and that the government of the Bharatiya Janata Party (BJP)-led coalition at that time was equally culpable.\n\
		If the BJP decides to make a song and dance about one part of Raja's statement, then the other part of his statement squarely indicts Atal Bihari Vajpayee also,' Congress spokesperson Manish Tewari said.\n\
		The official probe agency has said that Raja's decision as telecom minister in 2008 to issue radio spectrum to companies at a mere Rs.1,659 crore ($350 million) for a pan-India operation had caused the exchequer losses worth thousands of crores of rupees.\n\
		Nine new telecom companies were issued the radio frequency airwaves, a scarce national resource, to operate second generation (2G) mobile phone services in the country. As many as 122 circle-wise licences were issued.\n\
		The probe agency questioned the manner in which allocations were made that even resulted in a windfall for some.\n\
		A new player Swan Telecom had bought licences for 13 circles with the necessary spectrum for $340 million but managed to sell a 45 percent stake in the company to UAE's Etisalat for $900 million. This swelled its valuation to $2 billion without a single subscriber.\n\
		Similarly, another new player, Unitech, paid $365 million as licence fee but sold a 60 percent stake to Norway's Telenor for $1.36 billion, taking its valuation to nearly $2 billion, again without a single subscriber.\n\
		\n\
		The MBR can only represent four partitions. A technique called \"extended\" partitioning is used to allow more than four, and often times it is used when there are more than two partitions. All we're going to say about extended partitions is that they appear in this table just like a normal partition, and their first sector has another partition table that describes the partitions within its space. But for the sake of simply getting some code to work, we're going to not worry about extended partitions (and repartition and reformat any drive that has them....) The most common scenario is only one partition using the whole drive, with partitions 2, 3 and 4 blank.";	

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WConsoleLog", 
										sText.c_str(),
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										397, 
										163,
										hParent, 
										HMENU(198), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTextBox", 
										sText.c_str(), 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										319, 
										165,
										hParent, 
										HMENU(197), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTextBox", 
										sText.c_str(), 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										600, 
										163,
										hParent, 
										HMENU(197), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTabbedPane", 
										"",
										WM_ANCHOR_TOPLEFT,
										iXPos,
										iYPos, 
										400, 
										360,
										hParent, 
										HMENU(1966), 
										NULL);
				((WTabbedPane*)hWnd)->setSelectedTab(1);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WListBox", 
										"", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										400, 
										140,
										hParent, 
										HMENU(1001), 
										NULL);
			{
				char* ss = new char[255];
				memset(ss, 0, 255);

				LISTBOX_ITEM* item;
				for(int i = 0; i < 20; i++) {
					sprintf(ss, "item %d", i);

					item = new LISTBOX_ITEM();
					item->itemLabel = ss;

					((WListBox*)hWnd)->addItem(item);
				}
				sprintf(ss, "itemmmmmmmmmmmmmmm ieee");
				item = new LISTBOX_ITEM();
				item->itemLabel = ss;
				((WListBox*)hWnd)->addItem(item);
				sprintf(ss, "The FAT filesystems are designed to handle bad sectors aalekh.");
				item = new LISTBOX_ITEM();
				item->itemLabel = ss;
				((WListBox*)hWnd)->addItem(item);
				((WListBox*)hWnd)->removeItemAt(10);
				((WListBox*)hWnd)->setSelectedIndex(5);

				((WListBox*)hWnd)->setComponentAsChild(true);

				delete[] ss;
			}

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WComboBox", 
										"", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										250, 
										100,
										hParent, 
										HMENU(IDC_CB_FONT), 
										NULL);
			{
				((WComboBox*)hWnd)->addItem("Rosemary_DroidSans");
				((WComboBox*)hWnd)->addItem("Crisp");
				((WComboBox*)hWnd)->addItem("Consolas");
				((WComboBox*)hWnd)->addItem("Walkway_Black");
				((WComboBox*)hWnd)->addItem("Rosemary Roman");
				((WComboBox*)hWnd)->addItem("Kingdom_Hearts_Font");
				((WComboBox*)hWnd)->addItem("Inspyratta");
				((WComboBox*)hWnd)->addItem("DroidSansMono");
				((WComboBox*)hWnd)->addItem("diagoth");
				((WComboBox*)hWnd)->addItem("DejaVuSans");
				((WComboBox*)hWnd)->addItem("Comic Sans MS");
				((WComboBox*)hWnd)->addItem("ROSEMARY_DROIDSANS-BOLD");
				((WComboBox*)hWnd)->setSelectedIndex(0);
			}

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTextBox", 
										sText.c_str(), 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										800, 
										500,
										hParent, 
										HMENU(1111), 
										NULL);
			((WTextBox*)hWnd)->showLineNumbers(true);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTree", 
										"Title2", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										450, 
										450,
										hParent, 
										HMENU(1001), 
										NULL);

			iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
			hWnd = 
			CreateComponentEx(	"WTable", 
										"Table", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										450, 
										450,
										hParent, 
										HMENU(1002), 
										NULL);
				((WTable*)hWnd)->addColumn("Name");
				((WTable*)hWnd)->addColumn("Date Modified");
				((WTable*)hWnd)->addColumn("Type");
				((WTable*)hWnd)->addColumn("Size");
				((WTable*)hWnd)->addColumn("Date Created");
				((WTable*)hWnd)->addColumn("Authors");
				((WTable*)hWnd)->addColumn("Tags");

				TableRowData* trd = NULL;
				TableCellData* tcd = NULL;
				for(int ii = 0; ii < 15; ii++) {
					trd = new TableRowData();
					{
						tcd = new TableCellData("1.txt");	trd->addCellData(tcd);
						tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
						tcd = new TableCellData("Type 0");	trd->addCellData(tcd);
						tcd = new TableCellData("12345");	trd->addCellData(tcd);
						tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
						tcd = new TableCellData("Aalekh Maldikar");	trd->addCellData(tcd);
						tcd = new TableCellData("RW");	trd->addCellData(tcd);
					}
					((WTable*)hWnd)->addRow(trd);
				}

				for(int ii = 0; ii < 15; ii++) {
					trd = new TableRowData();
					{
						tcd = new TableCellData("2.txt");	trd->addCellData(tcd);
						tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
						tcd = new TableCellData("Type 1");	trd->addCellData(tcd);
						tcd = new TableCellData("54321");	trd->addCellData(tcd);
						tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
						tcd = new TableCellData("Rashmi Maldikar");	trd->addCellData(tcd);
						tcd = new TableCellData("RW");	trd->addCellData(tcd);
					}
					((WTable*)hWnd)->addRow(trd);
				}

			iXPos = 500;
			iYPos = 40;
			/////////////////////////////////////////////////
			H_WND hWnd0 = 
			CreateComponentEx(	"WWindow", 
										"Title", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										500, 
										650,
										hParent, 
										HMENU(1212), 
										(LPVOID)ID_TYPE_WND_C);
			((WWindow*)hWnd0)->setVisible(true);
			{
					iXPos = 20;
					iYPos = 40;
					hWnd = 
					CreateComponentEx(	"WButton", 
												"Simple Button !!!", 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												125, 
												25,
												hWnd0, 
												HMENU(1101), 
												(LPVOID)"Button");
						((WButton*)hWnd)->setComponentAsChild(true);
						H_WND hButton = hWnd;
					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
					hWnd = 
					CreateComponentEx(	"WCheckbox", 
												"CheckBox text111", 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												100, 
												25,
												hWnd0, 
												HMENU(1190), 
												NULL);
					((WComponent*)hWnd)->setAlignmentParent((WComponent*)hButton);
					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
						hWnd = 
						CreateComponentEx(	"WTextField", 
													"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
													WM_ALIGN_WRT_PARENT_CENTERRIGHT | WM_ANCHOR_TOPCENTER,
													iXPos,
													iYPos, 
													200, 
													23,
													hWnd0, 
													HMENU(99), 
													NULL);
						((WTextField*)hWnd)->setComponentAsChild(true);

					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
						hWnd = 
						CreateComponentEx(	"WTextBox", 
													sText.c_str(), 
													WM_ANCHOR_TOPLEFT, 
													iXPos,
													iYPos, 
													250, 
													163,
													hWnd0, 
													HMENU(99), 
													NULL);
						((WTextBox*)hWnd)->showLineNumbers(true);

					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
						hWnd = 
						CreateComponentEx(	"WTextBox", 
													sText.c_str(), 
													WM_ANCHOR_TOPLEFT, 
													iXPos,
													iYPos, 
													260, 
													165,
													hWnd0, 
													HMENU(199), 
													NULL);

					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
						hWnd = 
						CreateComponentEx(	"WComboBox", 
													"", 
													WM_ANCHOR_TOPLEFT, 
													iXPos,
													iYPos, 
													250, 
													100,
													hWnd0, 
													HMENU(299), 
													NULL);
						((WComboBox*)hWnd)->addDefaultTestItems();

					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
					{
						hWnd = 
						CreateComponentEx(	"WListBox", 
													"", 
													WM_ANCHOR_TOPLEFT, 
													iXPos,
													iYPos, 
													400, 
													140,
													hWnd0, 
													HMENU(1001), 
													(LPVOID)!true);
						char* ss1 = new char[255];
						memset(ss1, 0, 255);

						LISTBOX_ITEM* item;
						for(int i = 0; i < 20; i++) {
							sprintf(ss1, "item %d", i);

							item = new LISTBOX_ITEM();
							item->itemLabel = ss1;

							((WListBox*)hWnd)->addItem(item);
						}
						sprintf(ss1, "itemmmmmmmmmmmmmmm ieee");
						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;
						((WListBox*)hWnd)->addItem(item);
						sprintf(ss1, "The FAT filesystems are designed to handle bad sectors aalekh.");
						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;
						((WListBox*)hWnd)->addItem(item);
						((WListBox*)hWnd)->removeItemAt(10);
						((WListBox*)hWnd)->setSelectedIndex(5);

						((WListBox*)hWnd)->setComponentAsChild(true);

						delete[] ss1;
					}

				H_WND hWndW0 = 
				CreateComponentEx(	"WWindow", 
											"Title1", 
											WM_ANCHOR_TOPLEFT, 
											300,
											40, 
											300, 
											550,
											hWnd0, 
											HMENU(1001), 
											(LPVOID)ID_TYPE_WND_CMX);
					{

						iXPos = 20;
						iYPos = 40;
							hWnd = 
							CreateComponentEx(	"WTextField", 
														"00 I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														220, 
														23,
														hWndW0, 
														HMENU(99), 
														NULL);
							((WTextField*)hWnd)->setComponentAsChild(true);

						iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
							hWnd = 
							CreateComponentEx(	"WComboBox", 
														"", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														200, 
														100,
														hWndW0, 
														HMENU(299), 
														NULL);
							((WComboBox*)hWnd)->addDefaultTestItems();

						iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
						{
							H_WND hWind1 = 
							CreateComponentEx(	"WWindow", 
														"Title1", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														250, 
														400,
														hWndW0, 
														HMENU(1001), 
														(LPVOID)ID_TYPE_WND_CMX);

							iXPos = 20;
							iYPos = 40;
							hWnd = 
							CreateComponentEx(	"WTextBox", 
														sText.c_str(), 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														200, 
														163,
														hWind1, 
														HMENU(199), 
														NULL);
								((WTextBox*)hWnd)->showLineNumbers(true);

							iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
							hWnd = 
							CreateComponentEx(	"WWindow", 
														"Title1", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														200, 
														100,
														hWind1, 
														HMENU(1001), 
														(LPVOID)ID_TYPE_WND_C);
						}
					}
			}

			iXPos = ((WComponent*)hWnd0)->getOffsetX() + ((WComponent*)hWnd0)->getWidth() + 10;
			iYPos = 40;
			/////////////////////////////////////////////////
			H_WND wind0 = 
			CreateComponentEx(	"WWindow", 
										"Title", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										500, 
										650,
										hParent, 
										HMENU(1011), 
										(LPVOID)ID_TYPE_WND_C);
			((WWindow*)wind0)->setVisible(true);
			{
				/////////////////////////////////////////////////
				iXPos = 20;
				iYPos = 40;
				hWnd = 
				CreateComponentEx(	"WButton", 
											"SSSSimple Button", 
											WM_ANCHOR_TOPLEFT, 
											iXPos,
											iYPos, 
											125, 
											25,
											wind0, 
											HMENU(111), 
											(LPVOID)"Button");
				((WButton*)hWnd)->setComponentAsChild(true);

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
				hWnd = 
				CreateComponentEx(	"WCheckbox", 
											"CheckBox text", 
											WM_ANCHOR_TOPLEFT, 
											iXPos,
											iYPos, 
											100, 
											25,
											wind0, 
											HMENU(111), 
											NULL);

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
				hWnd = 
				CreateComponentEx(	"WTextField", 
												"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												200, 
												23,
												wind0, 
												HMENU(99), 
												NULL);
				((WTextField*)hWnd)->setComponentAsChild(true);

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
				WTextBox* tbW1 = new WTextBox();
					hWnd = 
					CreateComponentEx(	"WTextBox", 
														sText.c_str(), 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														250, 
														163,
														wind0, 
														HMENU(199), 
														NULL);
				((WTextBox*)hWnd)->showLineNumbers(true);

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
					hWnd = 
					CreateComponentEx(	"WTextBox", 
												sText.c_str(), 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												260, 
												165,
												wind0, 
												HMENU(199), 
												NULL);

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
					hWnd = 
					CreateComponentEx(	"WComboBox", 
																		"", 
																		WM_ANCHOR_TOPLEFT, 
																		iXPos,
																		iYPos, 
																		250, 
																		100,
																		wind0, 
																		HMENU(299), 
																		NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
				{
					hWnd = 
					CreateComponentEx(	"WListBox", 
												"", 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												400, 
												140,
												wind0, 
												HMENU(1001), 
												(LPVOID)!true);
					char* ss1 = new char[255];
					memset(ss1, 0, 255);

					LISTBOX_ITEM* item;
					for(int i = 0; i < 20; i++) {
						sprintf(ss1, "item %d", i);

						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;

						((WListBox*)hWnd)->addItem(item);
					}
					sprintf(ss1, "itemmmmmmmmmmmmmmm ieee");
					item = new LISTBOX_ITEM();
					item->itemLabel = ss1;
					((WListBox*)hWnd)->addItem(item);
					sprintf(ss1, "The FAT filesystems are designed to handle bad sectors aalekh.");
					item = new LISTBOX_ITEM();
					item->itemLabel = ss1;
					((WListBox*)hWnd)->addItem(item);
					((WListBox*)hWnd)->removeItemAt(10);
					((WListBox*)hWnd)->setSelectedIndex(5);

					((WListBox*)hWnd)->setComponentAsChild(true);

					delete[] ss1;
				}

				H_WND windW0 = 
					CreateComponentEx(	"WWindow", 
												"Title1", 
												WM_ANCHOR_TOPLEFT, 
												300,
												40, 
												300, 
												550,
												wind0, 
												HMENU(1001), 
												(LPVOID)ID_TYPE_WND_CMX);

					iXPos = 20;
					iYPos = 40;
					hWnd = 
					CreateComponentEx(	"WTextField", 
														"00 I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														220, 
														23,
														windW0, 
														HMENU(99), 
														NULL);
					((WTextField*)hWnd)->setComponentAsChild(true);

					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
					hWnd = 
					CreateComponentEx(	"WComboBox", 
																		"", 
																		WM_ANCHOR_TOPLEFT, 
																		iXPos,
																		iYPos, 
																		200, 
																		100,
																		windW0, 
																		HMENU(299), 
																		NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();

					iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
					H_WND hwind1 = 
					CreateComponentEx(	"WWindow", 
												"Title1", 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												250, 
												400,
												windW0, 
												HMENU(1001), 
												(LPVOID)ID_TYPE_WND_CMX);
			
					hWnd = 
					CreateComponentEx(	"WWindow", 
												"Title1", 
												WM_ANCHOR_TOPLEFT, 
												iXPos,
												iYPos, 
												250, 
												400,
												windW0, 
												HMENU(1001), 
												(LPVOID)ID_TYPE_WND_CMX);

						iXPos = 20;
						iYPos = 40;
						hWnd = 
						CreateComponentEx(	"WTextBox", 
														sText.c_str(), 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														200, 
														163,
														hwind1, 
														HMENU(199), 
														NULL);
						((WTextBox*)hWnd)->showLineNumbers(true);

						iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
						hWnd = 
						CreateComponentEx(	"WWindow", 
														"Title1", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														200, 
														100,
														hwind1, 
														HMENU(1001), 
														(LPVOID)ID_TYPE_WND_C);
				/////////////////////////////////////////////////
			/////////////////////////////////////////////////
			}

			iXPos = ((WComponent*)wind0)->getOffsetX() + ((WComponent*)wind0)->getWidth() + 10;
			iYPos = 40;
			H_WND hwind1 = 
			CreateComponentEx(	"WWindow", 
											"Title1", 
											WM_ANCHOR_TOPLEFT, 
											iXPos,
											iYPos, 
											200, 
											550,
											hParent, 
											HMENU(1001), 
											(LPVOID)ID_TYPE_WND_CX);
			{
				iXPos = 20;
				iYPos = 40;
				hWnd = 
				CreateComponentEx(	"WTextField", 
														"100 I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
														WM_ANCHOR_TOPLEFT, 
														iXPos,
														iYPos, 
														150, 
														23,
														hwind1, 
														HMENU(99), 
														NULL);
					((WTextField*)hWnd)->setComponentAsChild(true);

				iYPos = ((WComponent*)hWnd)->getOffsetY() + ((WComponent*)hWnd)->getHeight() + 5;
				hWnd = 
				CreateComponentEx(	"WComboBox", 
																		"", 
																		WM_ANCHOR_TOPLEFT, 
																		iXPos,
																		iYPos, 
																		250, 
																		100,
																		hwind1, 
																		HMENU(299), 
																		NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();
			}

			iXPos = ((WComponent*)wind0)->getOffsetX();
			iYPos = ((WComponent*)wind0)->getOffsetY() + ((WComponent*)wind0)->getHeight() + 5;
			H_WND wind2 = 
			CreateComponentEx(	"WFrame", 
													"Title2", 
													WM_ANCHOR_TOPLEFT, 
													iXPos,
													iYPos, 
													550, 
													650,
													hParent, 
													HMENU(1113), 
													(LPVOID)true);
				((WFrame*)wind2)->setBorderVisibility(true);
				{
					hWnd = 
					CreateComponentEx(	"WButton", 
												"Simple Button", 
												WM_ANCHOR_TOPLEFT, 
												20,
												40, 
												125, 
												25,
												wind2, 
												HMENU(111), 
												(LPVOID)"Button");
					((WButton*)hWnd)->setComponentAsChild(true);

					hWnd = 
					CreateComponentEx(	"WCheckbox", 
																"CheckBox text", 
																WM_ANCHOR_TOPLEFT, 
																20,
																70, 
																100, 
																25,
																wind2, 
																HMENU(111), 
																NULL);

					hWnd = 
					CreateComponentEx(	"WTextField", 
												"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
												WM_ANCHOR_TOPLEFT, 
												20,
												100, 
												200, 
												23,
												wind2, 
												HMENU(99), 
												NULL);
					((WTextField*)hWnd)->setComponentAsChild(true);

					hWnd = 
					CreateComponentEx(	"WTextBox", 
														sText.c_str(), 
														WM_ANCHOR_TOPLEFT, 
														20,
														130, 
														250, 
														163,
														wind2, 
														HMENU(199), 
														NULL);
					((WTextBox*)hWnd)->showLineNumbers(true);

					hWnd = 
					CreateComponentEx(	"WTextBox", 
													sText.c_str(), 
													WM_ANCHOR_TOPLEFT, 
													20,
													300, 
													260, 
													165,
													wind2, 
													HMENU(199), 
													NULL);

					hWnd = 
					CreateComponentEx(	"WComboBox", 
													"", 
													WM_ANCHOR_TOPLEFT, 
													20,
													470, 
													250, 
													100,
													wind2, 
													HMENU(299), 
													NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();

					hWnd = 
					CreateComponentEx(	"WListBox", 
														"", 
														WM_ANCHOR_TOPLEFT, 
														20,
														560, 
														400, 
														140,
														wind2, 
														HMENU(1001), 
														(LPVOID)!true);
					{
						char* ss1 = new char[255];
						memset(ss1, 0, 255);

						LISTBOX_ITEM* item;
						for(int i = 0; i < 20; i++) {
							sprintf(ss1, "item %d", i);

							item = new LISTBOX_ITEM();
							item->itemLabel = ss1;

							((WListBox*)hWnd)->addItem(item);
						}
						sprintf(ss1, "itemmmmmmmmmmmmmmm ieee");
						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;
						((WListBox*)hWnd)->addItem(item);
						sprintf(ss1, "The FAT filesystems are designed to handle bad sectors aalekh.");
						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;
						((WListBox*)hWnd)->addItem(item);
						((WListBox*)hWnd)->removeItemAt(10);
						((WListBox*)hWnd)->setSelectedIndex(5);

						((WListBox*)hWnd)->setComponentAsChild(true);

						delete[] ss1;
					}

					hWnd = 
					CreateComponentEx(	"WComboBox", 
														"", 
														WM_ANCHOR_TOPLEFT, 
														20,
														720, 
														250, 
														100,
														wind2, 
														HMENU(299), 
														NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();

					H_WND wind1 = 
					CreateComponentEx(	"WWindow", 
												"Title1", 
												WM_ANCHOR_TOPLEFT, 
												300,
												40, 
												200, 
												550,
												wind2, 
												HMENU(1001), 
												(LPVOID)ID_TYPE_WND_CX);
					{
						hWnd = 
						CreateComponentEx(	"WTextField", 
													"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
													WM_ANCHOR_TOPLEFT, 
													20,
													40, 
													150, 
													23,
													wind1, 
													HMENU(99), 
													NULL);
						((WTextField*)hWnd)->setComponentAsChild(true);

						hWnd = 
						CreateComponentEx(	"WComboBox", 
													"", 
													WM_ANCHOR_TOPLEFT, 
													20,
													70, 
													250, 
													100,
													wind1, 
													HMENU(299), 
													NULL);
						((WComboBox*)hWnd)->addDefaultTestItems();
					}

					hWnd = 
					CreateComponentEx(	"WTree", 
												"Title2", 
												WM_ANCHOR_TOPLEFT, 
												50,
												140, 
												450, 
												450,
												wind2, 
												HMENU(1001), 
												NULL);

					hWnd = 
					CreateComponentEx(	"WTable", 
												"Table", 
												WM_ANCHOR_TOPLEFT, 
												50,
												140, 
												450, 
												450,
												wind2, 
												HMENU(1003), 
												NULL);
					{
							((WTable*)hWnd)->addColumn("Name");
							((WTable*)hWnd)->addColumn("Date Modified");
							((WTable*)hWnd)->addColumn("Type");
							((WTable*)hWnd)->addColumn("Size");
							((WTable*)hWnd)->addColumn("Date Created");
							((WTable*)hWnd)->addColumn("Authors");
							((WTable*)hWnd)->addColumn("Tags");

							TableRowData* trd = new TableRowData();
							TableCellData* tcd = NULL;
							{
								tcd = new TableCellData("1.txt");	trd->addCellData(tcd);
								tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Type 0");	trd->addCellData(tcd);
								tcd = new TableCellData("12345");	trd->addCellData(tcd);
								tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Aalekh Maldikar");	trd->addCellData(tcd);
								tcd = new TableCellData("RW");	trd->addCellData(tcd);
							}
							((WTable*)hWnd)->addRow(trd);
					
							trd = new TableRowData();
							{
								tcd = new TableCellData("2.txt");	trd->addCellData(tcd);
								tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Type 1");	trd->addCellData(tcd);
								tcd = new TableCellData("54321");	trd->addCellData(tcd);
								tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
								tcd = new TableCellData("Rashmi Maldikar");	trd->addCellData(tcd);
								tcd = new TableCellData("RW");	trd->addCellData(tcd);
							}
							((WTable*)hWnd)->addRow(trd);
					}

					hWnd = 
					CreateComponentEx(	"WInspector", 
												"Title1", 
												WM_ANCHOR_TOPLEFT, 
												120,
												40, 
												350, 
												250,
												wind2, 
												HMENU(1013), 
												(LPVOID)true);
					{
						((WInspector*)hWnd)->addTab();
						((WInspector*)hWnd)->addTab();
						((WInspector*)hWnd)->addTab();
					}
				}

			iXPos = ((WComponent*)wind2)->getOffsetX();
			iYPos = ((WComponent*)wind2)->getOffsetY() + ((WComponent*)wind2)->getHeight() + 10;
			wind2 = 
			CreateComponentEx(	"WFrame", 
										"Title2", 
										WM_ANCHOR_TOPLEFT, 
										iXPos,
										iYPos, 
										550, 
										650,
										hParent, 
										HMENU(1112), 
										(LPVOID)true);
			{
				((WFrame*)wind2)->setBorderVisibility(true);
				{
					hWnd = 
					CreateComponentEx(	"WButton", 
												"Simple Button", 
												WM_ANCHOR_TOPLEFT, 
												20,
												40, 
												125, 
												25,
												wind2, 
												HMENU(111), 
												(LPVOID)"Button");
					((WButton*)hWnd)->setComponentAsChild(true);

					hWnd = 
					CreateComponentEx(	"WCheckbox", 
												"CheckBox text", 
												WM_ANCHOR_TOPLEFT, 
												20,
												70, 
												100, 
												25,
												wind2, 
												HMENU(111), 
												NULL);

					hWnd = 
					CreateComponentEx(	"WTextField", 
												"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
												WM_ANCHOR_TOPLEFT, 
												20,
												100, 
												200, 
												23,
												wind2, 
												HMENU(99), 
												NULL);
					((WTextField*)hWnd)->setComponentAsChild(true);

					hWnd = 
					CreateComponentEx(	"WTextBox", 
													sText.c_str(), 
													WM_ANCHOR_TOPLEFT, 
													20,
													130, 
													260, 
													163,
													wind2, 
													HMENU(199), 
													NULL);
				((WTextBox*)hWnd)->showLineNumbers(true);

					hWnd = 
					CreateComponentEx(	"WTextBox", 
														sText.c_str(), 
														WM_ANCHOR_TOPLEFT, 
														20,
														300, 
														260, 
														165,
														wind2, 
														HMENU(199), 
														NULL);

					hWnd = 
					CreateComponentEx(	"WComboBox", 
																	"", 
																	WM_ANCHOR_TOPLEFT, 
																	20,
																	470, 
																	250, 
																	100,
																	wind2, 
																	HMENU(299), 
																	NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();

					hWnd = 
					CreateComponentEx(	"WListBox", 
															"", 
															WM_ANCHOR_TOPLEFT, 
															20,
															560, 
															400, 
															140,
															wind2, 
															HMENU(1001), 
															(LPVOID)!true);
					{
						char* ss1 = new char[255];
						memset(ss1, 0, 255);

						LISTBOX_ITEM* item;
						for(int i = 0; i < 20; i++) {
							sprintf(ss1, "item %d", i);

							item = new LISTBOX_ITEM();
							item->itemLabel = ss1;

							((WListBox*)hWnd)->addItem(item);
						}
						sprintf(ss1, "itemmmmmmmmmmmmmmm ieee");
						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;
						((WListBox*)hWnd)->addItem(item);
						sprintf(ss1, "The FAT filesystems are designed to handle bad sectors aalekh.");
						item = new LISTBOX_ITEM();
						item->itemLabel = ss1;
						((WListBox*)hWnd)->addItem(item);
						((WListBox*)hWnd)->removeItemAt(10);
						((WListBox*)hWnd)->setSelectedIndex(5);

						((WListBox*)hWnd)->setComponentAsChild(true);

						delete[] ss1;
					}

					hWnd = 
					CreateComponentEx(	"WComboBox", 
																		"", 
																		WM_ANCHOR_TOPLEFT, 
																		20,
																		720, 
																		250, 
																		100,
																		wind2, 
																		HMENU(299), 
																		NULL);
					((WComboBox*)hWnd)->addDefaultTestItems();

					H_WND wind1 = 
					CreateComponentEx(	"WWindow", 
														"Title1", 
														WM_ANCHOR_TOPLEFT, 
														300,
														40, 
														200, 
														550,
														wind2, 
														HMENU(1001), 
														(LPVOID)ID_TYPE_WND_CX);
					{
						hWnd = 
						CreateComponentEx(	"WTextField", 
																		"I'm just messing around with thread hooks. I've got one now that displays the clipboard, if it is in CF_TEXT format, whenever the user pastes in my application. The problem I've run into is that it can get the clipboard data fine if I've copied it from another application, but if I copy it from my own, it pastes just fine on the screen, but when I retrieve the clipboard data, its garbled. Heres the code.", 
																		WM_ANCHOR_TOPLEFT, 
																		20,
																		40, 
																		150, 
																		23,
																		wind1, 
																		HMENU(99), 
																		NULL);
						((WTextField*)hWnd)->setComponentAsChild(true);

						hWnd = 
						CreateComponentEx(	"WComboBox", 
																		"", 
																		WM_ANCHOR_TOPLEFT, 
																		20,
																		70, 
																		250, 
																		100,
																		wind1, 
																		HMENU(299), 
																		NULL);
						((WComboBox*)hWnd)->addDefaultTestItems();
					}

						hWnd = 
						CreateComponentEx(	"WTree", 
													"Title2", 
													WM_ANCHOR_TOPLEFT, 
													50,
													140, 
													450, 
													450,
													wind2, 
													HMENU(1001), 
													NULL);

						hWnd = 
						CreateComponentEx(	"WTable", 
													"Table", 
													WM_ANCHOR_TOPLEFT, 
													50,
													140, 
													450, 
													450,
													wind2, 
													HMENU(1004), 
													NULL);
					{
							((WTable*)hWnd)->addColumn("Name");
							((WTable*)hWnd)->addColumn("Date Modified");
							((WTable*)hWnd)->addColumn("Type");
							((WTable*)hWnd)->addColumn("Size");
							((WTable*)hWnd)->addColumn("Date Created");
							((WTable*)hWnd)->addColumn("Authors");
							((WTable*)hWnd)->addColumn("Tags");

							TableRowData* trd = NULL;
							TableCellData* tcd = NULL;
							for(int ii = 0; ii < 15; ii++) {
								trd = new TableRowData();
								{
									tcd = new TableCellData("1.txt");	trd->addCellData(tcd);
									tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
									tcd = new TableCellData("Type 0");	trd->addCellData(tcd);
									tcd = new TableCellData("12345");	trd->addCellData(tcd);
									tcd = new TableCellData("10 June 2012");	trd->addCellData(tcd);
									tcd = new TableCellData("Aalekh Maldikar");	trd->addCellData(tcd);
									tcd = new TableCellData("RW");	trd->addCellData(tcd);
								}
								((WTable*)hWnd)->addRow(trd);
							}
					
							for(int ii = 0; ii < 15; ii++) {
								trd = new TableRowData();
								{
									tcd = new TableCellData("2.txt");	trd->addCellData(tcd);
									tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
									tcd = new TableCellData("Type 1");	trd->addCellData(tcd);
									tcd = new TableCellData("54321");	trd->addCellData(tcd);
									tcd = new TableCellData("12 June 2012");	trd->addCellData(tcd);
									tcd = new TableCellData("Rashmi Maldikar");	trd->addCellData(tcd);
									tcd = new TableCellData("RW");	trd->addCellData(tcd);
								}
								((WTable*)hWnd)->addRow(trd);
							}
					}

					hWnd = 
					CreateComponentEx(	"WInspector", 
															"Title1", 
															WM_ANCHOR_TOPLEFT, 
															120,
															40, 
															350, 
															250,
															wind2, 
															HMENU(1013), 
															(LPVOID)true);
					{
						((WInspector*)hWnd)->addTab();
						((WInspector*)hWnd)->addTab();
						((WInspector*)hWnd)->addTab();
					}
				}
			}
		}

		WWidgetManager*							m_pWidgetManager = nullptr;
		UI_FUNC_CALLBACK						m_fUIFuncCallback;

		MOUSE_FUNC_CALLBACK						m_fMouseFuncCallback;
		KEY_FUNC_CALLBACK						m_fKeyboardFuncCallback;
		UNICODE_CHAR_FUNC_CALLBACK				m_fUnicodeCharFuncCallback;
};

int main(int argc, char** argv)
{
	YAGui yaGui;
	exit(EXIT_SUCCESS);
}

/*
int main(int argc, char** argv)
{
	static const struct
	{
		float x, y;
		float r, g, b;
	}vertices[3]
	{
		{-0.5f, -0.5f,	1.0f, 0.0f, 0.0f},
		{0.5f, -0.5f,	0.0f, 1.0f, 0.0f},
		{0.0f, 0.5f,	0.0f, 0.0f, 1.0f}
	};

	//float vertices[] = 
	//{
	//	-0.5f, -0.5f, 0.0f,
	//	0.5f, -0.5f, 0.0f,
	//	0.0f, 0.5f, 0.0f
	//};

	static const char* vertex_shader_text =
	"#version 330 core\n"
	"layout (location = 0) in vec2 vPos;\n"
	"layout (location = 1) in vec3 vCol;\n"
	"out vec3 color;\n"
	"void main()\n"
	"{\n"
		"gl_Position = vec4(vPos, 0.0, 1.0);\n"
		"color = vCol\n;"
	"}\n";

	static const char* fragment_shader_text =
	"#version 330 core\n"
	"in vec3 color;\n"
	"out vec4 FragColor;\n"
	"void main()\n"
	"{\n"
		"FragColor = vec4(color, 1.0);\n"
	"}\n";

	//static const char* vertex_shader_text =
	//"#version 330 core\n"
	//"layout (location = 0) in vec3 aPos;\n"
	//"void main()\n"
	//"{\n"
	//	"gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	//"}\n";
	//
	//static const char* fragment_shader_text =
	//"#version 330 core\n"
	//"out vec4 FragColor;\n"
	//"void main()\n"
	//"{\n"
	//	"FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0);\n"
	//"}\n";

	{
		GLuint VBO, iVS, iFS, iProgram;
		unsigned int VAO;

		if (!glfwInit())
		{
			exit(EXIT_FAILURE);
		}

		// Enable OpenGL3.3 Core
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		GLFWwindow* pGLFWwindow = glfwCreateWindow(1024, 1024, "Temp", nullptr, nullptr);
		if (!pGLFWwindow)
		{
			exit(EXIT_FAILURE);
		}

		glfwMakeContextCurrent(pGLFWwindow);
		// glad: load all OpenGL function pointers
		// ---------------------------------------
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return -1;
		}
		glfwSwapInterval(1);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);

		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		iVS = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(iVS, 1, &vertex_shader_text, NULL);
		glCompileShader(iVS);

		iFS = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(iFS, 1, &fragment_shader_text, NULL);
		glCompileShader(iFS);

		iProgram = glCreateProgram();
		glAttachShader(iProgram, iVS);
		glAttachShader(iProgram, iFS);
		glLinkProgram(iProgram);

		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)0);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertices[0]), (void*)(sizeof(float) * 2));
		glEnableVertexAttribArray(1);

		//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		//glEnableVertexAttribArray(0);

		while(!glfwWindowShouldClose(pGLFWwindow))
		{
			glViewport(0, 0, 1024, 1024);
			glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			{
				glUseProgram(iProgram);
				
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glDrawArrays(GL_TRIANGLES, 0, 3);
			}

			glfwSwapBuffers(pGLFWwindow);
			glfwPollEvents();
		}

		glfwDestroyWindow(pGLFWwindow);
		glfwTerminate();
	}
}
//*/

/*
//hello_triangle.cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in vec3 aPos;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
"}\0";
const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
"}\n\0";

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}


	// build and compile our shader program
	// ------------------------------------
	// vertex shader
	int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	char infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// fragment shader
	int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}
	// link shaders
	int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		-0.5f, -0.5f, 0.0f, // left  
		 0.5f, -0.5f, 0.0f, // right 
		 0.0f,  0.5f, 0.0f  // top   
	};

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);


	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// draw our first triangle
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
		glDrawArrays(GL_TRIANGLES, 0, 3);
		// glBindVertexArray(0); // no need to unbind it every time 

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*/

/*
#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <Common/shader.h>
#include <iostream>
#include "Engine/Texture.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// build and compile our shader zprogram
	// ------------------------------------
	Shader ourShader("../res/UI/4.2.texture.vs", "../res/UI/4.2.texture.fs");

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	//float vertices[] = {
	//	// positions          // colors           // texture coords
	//	 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
	//	 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
	//	-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
	//	-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
	//};
	float vertices[] = {
		// positions          // colors           // texture coords
		0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left

		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f,  // top left 
		0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f // bottom left

		//0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		//-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		//-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		1, 2, 3  // second triangle
	};
	unsigned int VBO, VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);


	// load and create a texture 
	// -------------------------
	Texture* pTexture1 = Texture::createEx("../res/UI/container.jpg", false);
	if (pTexture1 == NULL)
		return 1;

	Texture* pTexture2 = Texture::createEx("../res/UI/awesomeface.png", false);
	if (pTexture2 == NULL)
		return 1;

	// tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
	// -------------------------------------------------------------------------------------------
	ourShader.use(); // don't forget to activate/use the shader before setting uniforms!
	// either set it manually like so:
	glUniform1i(glGetUniformLocation(ourShader.ID, "texture1"), 0);
	// or set it via the texture class
	ourShader.setInt("texture2", 1);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, pTexture1->getHandle());
		pTexture1->bind();
		glActiveTexture(GL_TEXTURE1);
		//glBindTexture(GL_TEXTURE_2D, pTexture2->getHandle());
		pTexture2->bind();

		// render container
		ourShader.use();
		//glBindVertexArray(VAO);
		//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// OR
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		//glDrawArrays(GL_QUADS, 0, 6);

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// glfw: terminate, clearing all previously allocated GLFW resources.
	// ------------------------------------------------------------------
	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//*/