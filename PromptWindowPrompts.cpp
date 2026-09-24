#pragma once



#include "GlobalVariables.h"
#include "FileHandling.cpp"

 

//void AnswerQuestion(int question, int answer, GLFWwindow* wind);

void ChangeTheme(int theme);

void ClickFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain);

void RightClickFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain);

void HoldClickFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain);

void RightHoldFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain);






void ClickFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain)
{
	Vector2 clickFloat = clickPos;

	clickPos.x = int(clickPos.x);
	clickPos.y = int(clickPos.y);




	if (clickPos.y == 0)
	{
		if (clickPos.x == wind->size.x - 2 || clickPos.x == wind->size.x - 1) // Exit window.
		{
			if (wind->name == "Instrument Editor")
				sampleDisplay.visible = false;
			windowController.windows.erase(windowController.windows.begin() + windowIndex);
			windowController.windows.shrink_to_fit();
			gui.drawUIThisFrame = true;
			gui.drawFrameThisFrame = true;
			screen.mouseDown = false;
			return;
		}
		else // Drag window.
		{
			wind->dragWindow = true;
			wind->dragPoint.x = clickPos.x;
			wind->dragPoint.y = clickPos.y;
		}
		// Exit
	}
	
	{

		if (wind->name == "Themes")
		{
			if (clickPos.y > 0 && clickPos.y < 33)
			{
				ChangeTheme((int(clickPos.y) - 1));
				SaveSettings();
			}
			else if (clickPos.y == 34)
			{
				gui.lightMode = !gui.lightMode;
				LoadGUIThemes();
				ChangeTheme(gui.uiColorTheme);
				SaveSettings();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
		}
		else if (wind->name == "Save and Exit")
		{
			if (clickPos.y == 6)
			{
				SaveSong();
				glfwSetWindowShouldClose(windMain, true);
			}
			else if (clickPos.y == 8)
				glfwSetWindowShouldClose(windMain, true);
		}
		else if (wind->name == "Save and Load")
		{
			if (clickPos.y == 7)
			{
				SaveSong();
				LoadSong(editor.fileToLoad);
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				return;
			}
			else if (clickPos.y == 9)

			{
				loadedSong.unsavedChanges = false;
				LoadSong(editor.fileToLoad);
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				return;
			}
		}
		else if (wind->name == "Overwrite Song")
		{
			/*
			if (clickPos.y == 7)
			{
				loadedSong.overWriteOldSong = true;
				SaveSong();
				loadedSong.overWriteOldSong = false;
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				return;
			}
			else if (clickPos.y == 9)
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				return;
			}*/
		}
		else if (wind->name == "Load File")
		{

			if (clickPos.y == 1 && clickPos.x == 1)
			{
				fileNavigator.ExitFile();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 19 && clickPos.x > 28 && clickPos.x < 35) // Load.
			{
				if (editor.selectedFile < 0 || editor.selectedFile >= fileNavigator.fileNames.size())
					return;

				if (fileNavigator.fileNames[editor.selectedFile].at(0) == '1') // Load Song
				{
					gui.drawUIThisFrame = true;
					gui.drawFrameThisFrame = true;
					std::string fileN = fileNavigator.fileNames[editor.selectedFile];
					fileN.erase(0, 1);
					LoadSong(fileN);
				}
			}
			else if (clickPos.y > 0 && clickPos.x > 0 && clickPos.x < 39)
			{
				if (clickPos.y - 2 + fileNavigator.fileListScroll < fileNavigator.fileNames.size())
				{
					if (clickPos.x == 1)
					{
						if (fileNavigator.fileNames[clickPos.y - 2 + fileNavigator.fileListScroll].at(0) == '0')
						{
							std::string fileN = fileNavigator.fileNames[clickPos.y - 2 + fileNavigator.fileListScroll];
							fileN.erase(0, 1);
							fileNavigator.EnterFile(fileN);
						}
					}
					else
					{
						editor.selectedFile = clickPos.y - 2 + fileNavigator.fileListScroll;
					}
				}
			}
		}
		else if (wind->name == "Save Song" || wind->name == "Save Instrument" || wind->name == "Save Sample")
		{
			if (wind->name == "Save Song")
			{
				if (clickPos.y == 19 && clickPos.x == 26)
				{
					editor.deleteUnusedPatterns = !editor.deleteUnusedPatterns;
					return;
				}
			}
			if (clickPos.y == 1 && clickPos.x == 1)
			{
				fileNavigator.ExitFile();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 19 && clickPos.x > 28 && clickPos.x < 35) // Save.
			{
				if (wind->name == "Save Song")
					SaveSong();
				else if (wind->name == "Save Instrument")
					SaveCurrentInstrument();

				// Refresh the preset menu and file menu.
				presetMenu.NavigateToInstrumentType(presetMenu.categories[presetMenu.instrumentType]);
				fileNavigator.NavigateToFile();
			}
			else if (clickPos.y > 0 && clickPos.x > 0 && clickPos.x < 39)
			{
				if (clickPos.y - 2 + fileNavigator.fileListScroll < fileNavigator.fileNames.size())
				{
					if (clickPos.x == 1)
					{
						if (fileNavigator.fileNames[clickPos.y - 2 + fileNavigator.fileListScroll].at(0) == '0')
						{
							std::string fileN = fileNavigator.fileNames[clickPos.y - 2 + fileNavigator.fileListScroll];
							fileN.erase(0, 1);
							fileNavigator.EnterFile(fileN);
						}
					}
					else
					{
						editor.selectedFile = clickPos.y - 2 + fileNavigator.fileListScroll;
					}
				}
			}
		}
		else if (wind->name == "Settings")
		{
			if (clickPos.y == 2)
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				windowController.InitializeWindow("Themes", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 16, 36 });
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			if (clickPos.y == 4)
			{
				windowController.InitializeWindow("Fishtank", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 16, 13 });
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
		}
		else if (wind->name == "Selection")
		{
			if (clickPos.y == 2)
			{
				copyNotes();
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 4)
			{
				pasteNotes();
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 6)
			{
				deleteNotes();
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 8)
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				editor.transposeValue = 0;
				windowController.InitializeWindow("Transpose", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 16, 10 });
			}
			else if (clickPos.y == 10)
			{
				setNoteSamples();
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
		}
		else if (wind->name == "Transpose")
		{
			if (clickPos.y == 3)
			{
				if (clickPos.x == 2)
					editor.transposeValue += loadedSong.edo;
				if (clickPos.x == 4)
					editor.transposeValue -= loadedSong.edo;
			}
			else if (clickPos.y == 5)
			{
				if (clickPos.x == 2)
					editor.transposeValue++;
				if (clickPos.x == 4)
					editor.transposeValue--;
			}
			else if (clickPos.y == 9)
			{
				if (clickPos.x > 7 && clickPos.x < 15)
				{
					transposeNotes();
					gui.drawUIThisFrame = true;
					gui.drawFrameThisFrame = true;
				}
			}
		}
		else if (wind->name == "Export as .WAV")
		{
			if (clickPos.y == 1 && clickPos.x == 1)
			{
				fileNavigator.ExitFile();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 19 && clickPos.x > 28 && clickPos.x < 37) // Save.
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				for (int name = 0; name < fileNavigator.fileNames.size(); name++)
				{
					std::string indexName = fileNavigator.fileNames[name];
					indexName.erase(0, 1);
					if (indexName == loadedSong.songName + ".wav")
					{
						windowController.InitializeWindow("Overwrite WAV", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 20, 16 });
						return;
					}
				}

				windowController.InitializeWindow("Exporting...", { 45, 20 }, { 10, 3 });

				editor.toRecordSong = true;
			}
			else if (clickPos.y > 0 && clickPos.x > 0 && clickPos.x < 39)
			{
				if (clickPos.y - 2 + fileNavigator.fileListScroll < fileNavigator.fileNames.size())
				{
					if (clickPos.x == 1)
					{
						if (fileNavigator.fileNames[clickPos.y - 2 + fileNavigator.fileListScroll].at(0) == '0')
						{
							std::string fileN = fileNavigator.fileNames[clickPos.y - 2 + fileNavigator.fileListScroll];
							fileN.erase(0, 1);
							fileNavigator.EnterFile(fileN);
						}
					}
					else
					{
						editor.selectedFile = clickPos.y - 2 + fileNavigator.fileListScroll;
					}
				}
			}
		}
		else if (wind->name == "Overwrite WAV")
		{
			if (clickPos.y == 7)
			{
				RecordSong();
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				return;
			}
			else if (clickPos.y == 9)
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
				return;
			}
		}
		else if (wind->name == "Instrument Editor")
		{

			if (!loadedInstruments[editor.selectedInstrument].enabled)
				return;


			// Change wave shape type
			if (clickPos.y == 4)
			{
				if (clickPos.x > 1 && clickPos.x < 10)
				{
					loadedInstruments[editor.selectedInstrument].waveforms[0].waveType = (clickPos.x - 2) / 2;
					DrawSampleDisplay();
					loadedSong.unsavedChanges = true;
					return;
				}
			}

			
			// Synth UI.
			if (clickPos.x > 1 && clickPos.x < 11)
			{
				if (clickPos.y == 2) // Open preset menu.
				{
					presetMenu.NavigateToInstrumentType(presetMenu.categories[0]);
					windowController.InitializeWindow("Presets", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 48, 18 });
					return;
				}
			}

			


			if (clickPos.y == 7) // Toggle sustain forever
			{
				if (clickPos.x == 6)
				{
					loadedInstruments[editor.selectedInstrument].waveforms[0].noSustain = !loadedInstruments[editor.selectedInstrument].waveforms[0].noSustain;
				}
			}
			else if (clickPos.y == 13) // Toggle sustain forever (modulator
			{
				if (clickPos.x == 6)
				{
					loadedInstruments[editor.selectedInstrument].waveforms[1].noSustain = !loadedInstruments[editor.selectedInstrument].waveforms[1].noSustain;
				}
			}


			if (clickPos.y > 21 && clickPos.y < 26)
			{
				if (clickPos.x > 16 && clickPos.x < 23)
				{
					sampleDisplay.selectedEnvelope = clickPos.y - 22;
					DrawSampleDisplay();
				}
			}


			if (clickPos.y > 25 && clickPos.y < 38 && editor.selectedInstrument > -1) // Sample display
			{
				
			}
			else if (clickPos.y > 37)
			{
				if (clickPos.x == 1 || clickPos.x == 2) // Play sample
				{
					if (!editor.playingSong) // Play the note sound.
					{
						channels[0].resetChannelEffects(true);
						StartNote(0, editor.selectedInstrument, 48);
						
						sampleDisplay.playingInstrument = true;
					}
				}
				else if (clickPos.x == 3 || clickPos.x == 4) // Pause sample
				{
					channels[0].playing = false;
					sampleDisplay.playingInstrument = false;
					DrawSampleDisplay();
				}

				if (clickPos.y == 38) // Set envelope scale
				{
					if (clickPos.x > 5 && clickPos.x < 17)
					{
						loadedInstruments[editor.selectedInstrument].envelopeScale /= 2.0f;
						if (loadedInstruments[editor.selectedInstrument].envelopeScale < 0.125f)
							loadedInstruments[editor.selectedInstrument].envelopeScale = 4.0f;


						DrawSampleDisplay();
						loadedSong.unsavedChanges = true;
						return;
					}
				}
			}

			return;
			
		}
		else if (wind->name == "Presets")
		{
			if (clickPos.x > 0 && clickPos.y > 0 && clickPos.x < 13 && clickPos.y < 13)
			{
				if (int(clickPos.y) % 2 == 1)
				{
					presetMenu.instrumentType = (clickPos.y - 1) / 2;
					presetMenu.NavigateToInstrumentType(presetMenu.categories[presetMenu.instrumentType]);
				}

				return;
			}
			if (clickPos.x > 14 && clickPos.y > 0 && clickPos.y < 17)
			{
				if (clickPos.x < 30)
				{
					presetMenu.selectedSample = clickPos.y - 1;
					return;
				}
				else if (clickPos.x < 48)
				{
					presetMenu.selectedSample = clickPos.y - 1 + 16;
					return;
				}
			}
			else if (clickPos.y == 17 && clickPos.x > 7 && clickPos.x < 15)
			{
				if (presetMenu.selectedSample < presetMenu.fileNames.size())
				{
					Instrument newInstrument;

					std::ifstream instrumentFile("Presets/" + presetMenu.categories[presetMenu.instrumentType] + "/" + presetMenu.fileNames[presetMenu.selectedSample] + ".inst", std::ios::binary | std::ios::in);


					if (instrumentFile.is_open())
					{
						newInstrument = ReadInstrument(&instrumentFile);
					}

					loadedInstruments[editor.selectedInstrument] = newInstrument;

					loadedInstruments[editor.selectedInstrument].name = presetMenu.fileNames[presetMenu.selectedSample];

					DrawSampleDisplay();

					instrumentFile.close();

					loadedSong.unsavedChanges = true;
				}
				
				return;
			}
		}
		else if (wind->name == "File")
		{
			if (clickPos.y == 2) // New
			{
				// New song
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();

				NewSong();

				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 4) // Save song
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();

				fileNavigator.NavigateToFile();
				windowController.InitializeWindow("Save Song", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 40, 20 });
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 6) // Save instrument
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();

				fileNavigator.NavigateToFile();
				windowController.InitializeWindow("Save Instrument", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 40, 20 });
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 8) // Load
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();

				fileNavigator.NavigateToFile();
				windowController.InitializeWindow("Load File", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 40, 20 });
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
			else if (clickPos.y == 10) // Export
			{
				windowController.windows.erase(windowController.windows.begin() + windowIndex);
				windowController.windows.shrink_to_fit();

				fileNavigator.NavigateToFile();
				windowController.InitializeWindow("Export as .WAV", { int(gui.hoveredTile.x), int(gui.hoveredTile.y) }, { 40, 20 });
				gui.drawUIThisFrame = true;
				gui.drawFrameThisFrame = true;
			}
		}
	}





	return;
}



void RightClickFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain)
{
	clickPos.x = int(clickPos.x);
	clickPos.y = int(clickPos.y);

	//std::lock_guard<std::shared_mutex> lock(mtx);



	if (clickPos.y > 0)
	{

		
	}


	if (windowIndex > 0) // Move the dragged window to front.
	{
		FloatingWindow moveWind = windowController.windows[windowIndex];
		windowController.windows.erase(windowController.windows.begin() + windowIndex);
		windowController.windows.emplace(windowController.windows.begin(), moveWind);
		windowController.windows.shrink_to_fit();
		windowIndex = 0;
	}

	return;
}




void HoldClickFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain)
{
	//std::lock_guard<std::shared_mutex> lock(mtx);

	


	if (clickPos.y > 0)
	{
		if (wind->name == "Save Song" || wind->name == "Save Instrument" || wind->name == "Load File" || wind->name == "Export as .WAV")
		{
			if (clickPos.x < 1.0f && clickPos.y > 1.0f && clickPos.y < 20.0f)
			{
				fileNavigator.fileScrollBar.position = (clickPos.y - 2.5f) / 17.0f;
				if (fileNavigator.fileScrollBar.position < 0.0f) fileNavigator.fileScrollBar.position = 0.0f;
				if (fileNavigator.fileScrollBar.position > 1.0f) fileNavigator.fileScrollBar.position = 1.0f;
				fileNavigator.fileListScroll = fileNavigator.fileScrollBar.position * fileNavigator.fileNames.size();
				gui.drawFrameThisFrame = true;
			}
		}
		else if (wind->name == "Help")
		{
			if (clickPos.x < 1.0f && clickPos.y > 1.0f && clickPos.y < 39.0f)
			{
				gui.helpWindowScrollBar.position = (clickPos.y - 2.5f) / 37.0f;
				if (gui.helpWindowScrollBar.position < 0.0f) gui.helpWindowScrollBar.position = 0.0f;
				if (gui.helpWindowScrollBar.position > 1.0f) gui.helpWindowScrollBar.position = 1.0f;
				gui.helpWindowScroll = gui.helpWindowScrollBar.position * helpPageText.size();
				gui.drawFrameThisFrame = true;
			}
		}
		else if (wind->name == "Instrument Editor")
		{
			if (!loadedInstruments[editor.selectedInstrument].enabled)
				return;

			if (clickPos.x > 7 && clickPos.x < 16)
			{
				if (int(clickPos.y) == 5) // Edit noise volume.
				{
					loadedInstruments[editor.selectedInstrument].waveforms[0].noiseVolume = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[0].noiseVolume < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].noiseVolume = 0.0f;
					if (loadedInstruments[editor.selectedInstrument].waveforms[0].noiseVolume > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].noiseVolume = 1.0f;

					loadedSong.unsavedChanges = true;

					return;

				}
				else if (int(clickPos.y) == 6) // Set sample octave.
				{
					loadedInstruments[editor.selectedInstrument].waveforms[0].octave = 15 - int(float(int((clickPos.x - 7) * 2.0f)));

					if (loadedInstruments[editor.selectedInstrument].waveforms[0].octave < 0)
						loadedInstruments[editor.selectedInstrument].waveforms[0].octave = 0;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[0].octave > 15)
						loadedInstruments[editor.selectedInstrument].waveforms[0].octave = 15;

					loadedSong.unsavedChanges = true;

					return;
				}
				else if (int(clickPos.y) == 7) // Set sample release.
				{

					loadedInstruments[editor.selectedInstrument].waveforms[0].release = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[0].release < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].release = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[0].release > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].release = 1.0f;

					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 8) // Set LFO depth.
				{

					loadedInstruments[editor.selectedInstrument].waveforms[0].lfoDepth = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[0].lfoDepth < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].lfoDepth = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[0].lfoDepth > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].lfoDepth = 1.0f;

					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 9) // Set LFO speed.
				{

					loadedInstruments[editor.selectedInstrument].waveforms[0].lfoSpeed = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[0].lfoSpeed < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].lfoSpeed = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[0].lfoSpeed > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[0].lfoSpeed = 1.0f;

					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 12) // Set sample octave. (Modulator)
				{
					loadedInstruments[editor.selectedInstrument].waveforms[1].octave = 15 - int(float(int((clickPos.x - 7) * 2.0f)));

					if (loadedInstruments[editor.selectedInstrument].waveforms[1].octave < 0)
						loadedInstruments[editor.selectedInstrument].waveforms[1].octave = 0;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[1].octave > 15)
						loadedInstruments[editor.selectedInstrument].waveforms[1].octave = 15;

					loadedSong.unsavedChanges = true;

					return;
				}
				else if (int(clickPos.y) == 13) // Set sample release. (Modulator)
				{

					loadedInstruments[editor.selectedInstrument].waveforms[1].release = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[1].release < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[1].release = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[1].release > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[1].release = 1.0f;

					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 14) // Set LFO depth. (Modulator)
				{

					loadedInstruments[editor.selectedInstrument].waveforms[1].lfoDepth = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[1].lfoDepth < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[1].lfoDepth = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[1].lfoDepth > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[1].lfoDepth = 1.0f;

					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 15) // Set LFO speed. (Modulator)
				{

					loadedInstruments[editor.selectedInstrument].waveforms[1].lfoSpeed = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].waveforms[1].lfoSpeed < 0.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[1].lfoSpeed = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].waveforms[1].lfoSpeed > 1.0f)
						loadedInstruments[editor.selectedInstrument].waveforms[1].lfoSpeed = 1.0f;

					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 18) // Set instrument volume.
				{

					loadedInstruments[editor.selectedInstrument].volume = (float(int((clickPos.x - 7) * 2.0f)) / 16.0f);

					if (loadedInstruments[editor.selectedInstrument].volume < 0.0f)
						loadedInstruments[editor.selectedInstrument].volume = 0.0f;
					else if (loadedInstruments[editor.selectedInstrument].volume > 1.0f)
						loadedInstruments[editor.selectedInstrument].volume = 1.0f;
					
					DrawSampleDisplay();
					loadedSong.unsavedChanges = true;
					return;
				}
				else if (int(clickPos.y) == 23) // Set arp speed.
				{
					loadedInstruments[editor.selectedInstrument].arpSpeed = (float(int((clickPos.x - 7))));
					loadedInstruments[editor.selectedInstrument].arpSpeed = int(float(int((clickPos.x - 7) * 2.0f))) + 1;

					if (loadedInstruments[editor.selectedInstrument].arpSpeed < 1)
						loadedInstruments[editor.selectedInstrument].arpSpeed = 1;
					else if (loadedInstruments[editor.selectedInstrument].arpSpeed > 16)
						loadedInstruments[editor.selectedInstrument].arpSpeed = 16;

					loadedSong.unsavedChanges = true;
					return;
				}
			}




			if (clickPos.x > 16 && clickPos.x < 24) // Carrier frequencies.
			{
				if (clickPos.y > 3 && clickPos.y < 18.0f) // Change frequencies.
				{
					if (int(clickPos.x) > 15)
					{
						int newFreqVal = (17.0f - clickPos.y) * 4.0f;
						if (newFreqVal < 0) newFreqVal = 0;
						else if (newFreqVal > 64) newFreqVal = 64;

						if (loadedInstruments[editor.selectedInstrument].waveforms[0].frequencies[int(clickPos.x) - 12 - 4] != newFreqVal)
						{
							loadedInstruments[editor.selectedInstrument].waveforms[0].frequencies[int(clickPos.x) - 12 - 4] = newFreqVal;


							loadedSong.unsavedChanges = true;
						}
					}
				}
			}
			


			if (clickPos.x > 25 && clickPos.x < 33)
			{
				// Set arp length.
				if (clickPos.y > 25.0f && clickPos.y < 26.0f) // Set arp length.
				{
					loadedInstruments[editor.selectedInstrument].arpLength = int(clickPos.x - 24.5f + 1.0f);
					if (loadedInstruments[editor.selectedInstrument].arpLength > 8)
						loadedInstruments[editor.selectedInstrument].arpLength = 8;
					else if (loadedInstruments[editor.selectedInstrument].arpLength < 1)
						loadedInstruments[editor.selectedInstrument].arpLength = 1;
					loadedSong.unsavedChanges = true;
				}
				else if (clickPos.y > 18.0f && clickPos.y < 27.0f) // Set arp pitches.
				{
					// 0 - 14
					if (clickPos.y > 12.0f && clickPos.y < 25.0f)
					{
						loadedInstruments[editor.selectedInstrument].arpPitches[int(clickPos.x) - 25] = int((25.0f - clickPos.y) * 4.0f);

						if (loadedInstruments[editor.selectedInstrument].arpPitches[int(clickPos.x) - 25] < 0)
							loadedInstruments[editor.selectedInstrument].arpPitches[int(clickPos.x) - 25] = 0;
						if (loadedInstruments[editor.selectedInstrument].arpPitches[int(clickPos.x) - 25] > 24)
							loadedInstruments[editor.selectedInstrument].arpPitches[int(clickPos.x) - 25] = 24;


						// Reset arpeggiation in channels using this instrument.
						for (int ch = 0; ch < 8; ch++)
						{
							if (channels[ch].instrument == editor.selectedInstrument)
							{
								channels[ch].arpTimer = 0.0f;
							}
						}

						loadedSong.unsavedChanges = true;
					}
				}
				else if (clickPos.y > 3 && clickPos.y < 18.0f) // Change frequencies.
				{
					if (int(clickPos.x) > 24)
					{
						int newFreqVal = (17.0f - clickPos.y) * 4.0f;
						if (newFreqVal < 0) newFreqVal = 0;
						else if (newFreqVal > 64) newFreqVal = 64;

						if (loadedInstruments[editor.selectedInstrument].waveforms[1].frequencies[int(clickPos.x) - 21 - 4] != newFreqVal)
						{
							loadedInstruments[editor.selectedInstrument].waveforms[1].frequencies[int(clickPos.x) - 21 - 4] = newFreqVal;


							loadedSong.unsavedChanges = true;
						}
					}
				}
			}




			if (clickPos.y > 25 && clickPos.y < 38 && editor.selectedInstrument > -1) // Sample display
			{
				if (clickPos.y > 26) // Create sample points.
				{
					float amp = 1.0f - (clickPos.y - 27.0f) / (37.0f - 27.0f);
					amp *= 255.0f;
					amp = int(amp);
					if (amp < 0.0f) amp = 0.0f;
					else if (amp > 255.0f) amp = 255.0f;

					int pos = int(gui.floatHoveredTile.x - windowController.windows[windowIndex].position.x - 1.0f);

					if (pos < 0) pos = 0;
					else if (pos > 31) pos = 31;

					if (sampleDisplay.selectedEnvelope == 0) // AMP
						loadedInstruments[editor.selectedInstrument].waveforms[0].envelope[pos] = int(amp);
					else if (sampleDisplay.selectedEnvelope == 1) // FM
						loadedInstruments[editor.selectedInstrument].waveforms[1].envelope[pos] = int(amp);
					else if (sampleDisplay.selectedEnvelope == 2) // FREQ
						loadedInstruments[editor.selectedInstrument].pitchEnvelope[pos] = int(amp);

					
					DrawSampleDisplay();

					return;
				}
			}


		}
		else if (wind->name == "Copy Instrument")
		{
			if (clickPos.y > 1.0f && clickPos.y < 10.0f)
			{
				if (clickPos.x < 1.0f)
				{
					instrumentFloatingWindow.instrumentListScrollBar.position = (clickPos.y - 2.5f) / 7.0f;
					if (instrumentFloatingWindow.instrumentListScrollBar.position < 0.0f) instrumentFloatingWindow.instrumentListScrollBar.position = 0.0f;
					if (instrumentFloatingWindow.instrumentListScrollBar.position > 1.0f) instrumentFloatingWindow.instrumentListScrollBar.position = 1.0f;
					instrumentFloatingWindow.instrumentListScroll = instrumentFloatingWindow.instrumentListScrollBar.position * (256.0f - 10.0f);
					gui.drawUIThisFrame = true;
				}
			}
		}
	}



	return;
}



void RightHoldFloatingWindow(FloatingWindow* wind, int windowIndex, Vector2 clickPos, GLFWwindow* windMain)
{
	clickPos.x = int(clickPos.x);
	clickPos.y = int(clickPos.y);


	

	if (wind->name == "Instrument Editor")
	{
		if (!loadedInstruments[editor.selectedInstrument].enabled)
			return;


		if (clickPos.y > 25 && clickPos.y < 38 && editor.selectedInstrument > -1) // Sample display
		{
			if (clickPos.y > 26) // Create sample points.
			{
				int pos = int(gui.floatHoveredTile.x - windowController.windows[windowIndex].position.x - 1.0f);

				if (pos < 0) pos = 0;
				else if (pos > 31) pos = 31;

				if (sampleDisplay.selectedEnvelope == 0) // AMP
					loadedInstruments[editor.selectedInstrument].waveforms[0].envelope[pos] = 255;
				else if (sampleDisplay.selectedEnvelope == 1) // FM
					loadedInstruments[editor.selectedInstrument].waveforms[1].envelope[pos] = 255;
				else if (sampleDisplay.selectedEnvelope == 2) // FREQ
					loadedInstruments[editor.selectedInstrument].pitchEnvelope[pos] = 127;


				DrawSampleDisplay();

				return;
			}
		}


	}

	



	return;
}




void ChangeTheme(int theme)
{
	gui.uiColorTheme = theme;

	int sizeX, sizeY, comps;
	glBindTexture(GL_TEXTURE_2D, gui.uiTexture);
	unsigned char* data;

	std::string currentPath;

	

	if (gui.lightMode)
		currentPath = fileNavigator.getRelativePath() + "/GUI/TilesLight.png";
	else
		currentPath = fileNavigator.getRelativePath() + "/GUI/Tiles.png";

	data = stbi_load(&currentPath[0], &sizeX, &sizeY, &comps, 3);

	if (!data)
	{
		std::cout << "Tiles not found. ";
	}

	GUITheme currentTheme = gui.themes[theme];
	/*
	if (gui.uiBrightMode)
	{
		for (int i = 0; i < 6; i++)
			currentTheme.uiColors[i] = gui.themes[theme].uiColors[5 - i];
	}*/

	for (int i = 0; i < 512 * 512; i++)
	{
		unsigned char* colPos = data + i * 3;
		if (colPos[0] == 0 && colPos[1] == 0 && colPos[2] == 0)
		{
			data[i * 3] = currentTheme.uiColors[0].r;
			data[i * 3 + 1] = currentTheme.uiColors[0].g;
			data[i * 3 + 2] = currentTheme.uiColors[0].b;
		}
		else if (colPos[0] == 0 && colPos[1] == 0 && colPos[2] == 40)
		{
			data[i * 3] = gui.themes[theme].uiColors[1].r * 0.5f + gui.themes[theme].uiColors[0].r * 0.5f;
			data[i * 3 + 1] = gui.themes[theme].uiColors[1].g * 0.5f + gui.themes[theme].uiColors[0].g * 0.5f;
			data[i * 3 + 2] = gui.themes[theme].uiColors[1].b * 0.5f + gui.themes[theme].uiColors[0].b * 0.5f;
		}
		else if (colPos[0] == 30)
		{
			data[i * 3] = currentTheme.uiColors[1].r;
			data[i * 3 + 1] = currentTheme.uiColors[1].g;
			data[i * 3 + 2] = currentTheme.uiColors[1].b;
		}
		else if (colPos[0] == 50)
		{
			data[i * 3] = currentTheme.uiColors[2].r;
			data[i * 3 + 1] = currentTheme.uiColors[2].g;
			data[i * 3 + 2] = currentTheme.uiColors[2].b;
		}
		else if (colPos[0] == 90)
		{
			data[i * 3] = currentTheme.uiColors[3].r;
			data[i * 3 + 1] = currentTheme.uiColors[3].g;
			data[i * 3 + 2] = currentTheme.uiColors[3].b;
		}
		else if (colPos[0] == 150)
		{
			data[i * 3] = currentTheme.uiColors[4].r;
			data[i * 3 + 1] = currentTheme.uiColors[4].g;
			data[i * 3 + 2] = currentTheme.uiColors[4].b;
		}
		else if (colPos[0] == 255 && colPos[1] == 255 && colPos[2] == 255)
		{
			data[i * 3] = currentTheme.uiColors[5].r;
			data[i * 3 + 1] = currentTheme.uiColors[5].g;
			data[i * 3 + 2] = currentTheme.uiColors[5].b;
		}
		// Accent colors
		else if (colPos[0] == 63 && colPos[1] == 100 && colPos[2] == 100)
		{
			data[i * 3] = currentTheme.uiColors[6].r;
			data[i * 3 + 1] = currentTheme.uiColors[6].g;
			data[i * 3 + 2] = currentTheme.uiColors[6].b;
		}
		else if (colPos[0] == 127 && colPos[1] == 190 && colPos[2] == 0)
		{
			data[i * 3] = currentTheme.uiColors[7].r;
			data[i * 3 + 1] = currentTheme.uiColors[7].g;
			data[i * 3 + 2] = currentTheme.uiColors[7].b;
		}
		else if (colPos[0] == 255 && colPos[1] == 255 && colPos[2] == 0)
		{
			data[i * 3] = currentTheme.uiColors[8].r;
			data[i * 3 + 1] = currentTheme.uiColors[8].g;
			data[i * 3 + 2] = currentTheme.uiColors[8].b;
		}
		else if (colPos[0] == 210 && colPos[1] == 210 && colPos[2] == 230)
		{
			data[i * 3] = gui.themes[theme].uiColors[5].r * 0.5f + gui.themes[theme].uiColors[4].r * 0.5f;
			data[i * 3 + 1] = gui.themes[theme].uiColors[5].g * 0.5f + gui.themes[theme].uiColors[4].g * 0.5f;
			data[i * 3 + 2] = gui.themes[theme].uiColors[5].b * 0.5f + gui.themes[theme].uiColors[4].b * 0.5f;
		}
	}

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	stbi_image_free(data);

	// Set the ui colors.
	gui.uiColors[0] = gui.themes[theme].uiColors[0].r / 255.0f;
	gui.uiColors[1] = gui.themes[theme].uiColors[0].g / 255.0f;
	gui.uiColors[2] = gui.themes[theme].uiColors[0].b / 255.0f;

	gui.uiColors[3] = gui.themes[theme].uiColors[1].r / 255.0f;
	gui.uiColors[4] = gui.themes[theme].uiColors[1].g / 255.0f;
	gui.uiColors[5] = gui.themes[theme].uiColors[1].b / 255.0f;

	gui.uiColors[6] = gui.themes[theme].uiColors[2].r / 255.0f;
	gui.uiColors[7] = gui.themes[theme].uiColors[2].g / 255.0f;
	gui.uiColors[8] = gui.themes[theme].uiColors[2].b / 255.0f;

	gui.uiColors[9] = gui.themes[theme].uiColors[3].r / 255.0f;
	gui.uiColors[10] = gui.themes[theme].uiColors[3].g / 255.0f;
	gui.uiColors[11] = gui.themes[theme].uiColors[3].b / 255.0f;

	gui.uiColors[12] = gui.themes[theme].uiColors[4].r / 255.0f;
	gui.uiColors[13] = gui.themes[theme].uiColors[4].g / 255.0f;
	gui.uiColors[14] = gui.themes[theme].uiColors[4].b / 255.0f;

	gui.uiColors[15] = gui.themes[theme].uiColors[5].r / 255.0f;
	gui.uiColors[16] = gui.themes[theme].uiColors[5].g / 255.0f;
	gui.uiColors[17] = gui.themes[theme].uiColors[5].b / 255.0f;


	gui.uiColors[45] = gui.themes[theme].uiColors[7].r / 255.0f;
	gui.uiColors[46] = gui.themes[theme].uiColors[7].g / 255.0f;
	gui.uiColors[47] = gui.themes[theme].uiColors[7].b / 255.0f;

	gui.uiColors[48] = gui.themes[theme].uiColors[7].r / 255.0f;
	gui.uiColors[49] = gui.themes[theme].uiColors[7].g / 255.0f;
	gui.uiColors[50] = gui.themes[theme].uiColors[7].b / 255.0f;

	gui.uiColors[51] = gui.themes[theme].uiColors[8].r / 255.0f;
	gui.uiColors[52] = gui.themes[theme].uiColors[8].g / 255.0f;
	gui.uiColors[53] = gui.themes[theme].uiColors[8].b / 255.0f;

	gui.uiColors[54] = gui.themes[theme].uiColors[9].r / 255.0f;
	gui.uiColors[55] = gui.themes[theme].uiColors[9].g / 255.0f;
	gui.uiColors[56] = gui.themes[theme].uiColors[9].b / 255.0f;

	

	if (sampleDisplay.visible)
	{
		DrawSampleDisplay();
		
		gui.drawFrameThisFrame = true;
		DrawFrameBorder();
		//DrawEverything();
		
	}
}