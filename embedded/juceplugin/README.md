Introduction on how to use pyo inside a Juce audio plugin.
========================================================== 

To use pyo inside a Juce audio plugin, first you need a working 
install of pyo (for python 2.7 at the moment) on your system.
After installing pyo, you can move on and create a Juce audio plugin project.

------------------------------------------------------------------------------
Step 1 - Create an Audio Plug-In project with the Projucer. 

------------------------------------------------------------------------------
Step 2 - Add pyo files to your project. You need to copy these files in your
project "Source" folder and add them in Projucer:

- from this folder:
  * PyoClass.cpp
  * PyoClass.h
- from the folder "embedded":
  * m_pyo.h

------------------------------------------------------------------------------
Step 3 - Make sure that your project includes flags for compiling 
and linking against Python. Within the Projucer, you can set extra
flags for your selected exporter. For example, on linux, if you select
"Linux Makefile", you can add extra compiler and linker flags. Add 
these in the specific fields:

Extra linker flags :

    `python-config --ldflags`

Extra compiler flags :

    `python-config --cflags`

On MacOS, the default compiler in Xcode is LLVM and it will complain about 
python-config command. All you have to do is to run these two commands (without
the backtick symbols) in a terminal window and copy the results in the appropriate
field (you can leave out "-Wstrict-prototypes" flag as it is not valid in C++.

On MacOS, add the Python framework to your project. In the field "Extra Frameworks",
add :

	Python


------------------------------------------------------------------------------
Step 4 - Make sure that your project links to the Steinberg's VST SDK.
VST SDK can be freely downloaded from Steinberg web site:
http://www.steinberg.net/en/company/developers/ .

Enter the path of your SDK in the field "VST3 SDK" in the "Global Search Paths"
window.

On MacOS, I managed to build both VST 2 and VST 3 plugins but only the VST 3 
plugin actually runs in Reaper. The VST 2 plugin crashes the app.

------------------------------------------------------------------------------
Step 5 - Create a python file, named "stereoDelay.py", with these lines in 
it:

    # Retrieve the stereo input of the plugin.
    st_input = Input([0,1])
    # Parameters to change.
    dtime = SigTo(.5, 0.05, .5)
    feed = SigTo(.5, 0.05, .5)
    # Simple process. Stereo delay -> reverb.
    st_delay = SmoothDelay(st_input, delay=dtime, feedback=feed)
    st_rev = WGVerb(st_delay, feedback=0.8, cutoff=4000, bal=0.25).out()

Save "stereoDelay.py" in your project "Source" folder, add it to the project 
(drag-and-drop to the File explorer in the Projucer) and ensure that the 
option "Binary Resources" is checked when you select "Source" in the File 
explorer. Save your project. This should add two files, "BinaryData.h" and 
"BinaryData.cpp", in the "JuceLibraryCode" folder.
 
------------------------------------------------------------------------------

**For all the remaining steps, don't forget to replace XXX by your plugin's
name.**

------------------------------------------------------------------------------

Step 6 - Edit Source/PluginProcessor.h

Include PyoClass definition:

    #include "PyoClass.h"

Add a Pyo object to the public attributes of the *XXXAudioProcessor* class:

    Pyo pyo;

------------------------------------------------------------------------------
Step 7 - Edit Source/PluginProcessor.cpp

Add these lines to *XXXAudioProcessor::prepareToPlay* method:

    pyo.setup(getTotalNumOutputChannels(), samplesPerBlock, sampleRate);
    pyo.exec(BinaryData::stereoDelay_py);

Replace the processing part of *XXXAudioProcessor::processBlock* method with this
line:

    pyo.process(buffer);

The processing part is the code after this comment:

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...

------------------------------------------------------------------------------

Here you go! Compile, Run and Enjoy!


Adding GUI controls to your plugin
----------------------------------

Now, we will add two sliders to control the delay time and the feedback
of our process.

------------------------------------------------------------------------------
Step 8 - Edit Source/PluginEditor.h

Add the inheritance to *Slider::Listener* to your *XXXAudioProcessorEditor*
class definition:

    class XXXAudioProcessorEditor  : public AudioProcessorEditor, 
                                     private Slider::Listener
 
Add the default callback function in the public attributes of the class:
    
    void sliderValueChanged(Slider* slider) override;

Create two sliders in the public attributes of the class:
    
    Slider p1;
    Slider p2;

------------------------------------------------------------------------------
Step 9 - Edit Source/PluginEditor.cpp

Set the sliders properties in the editor constructor function named
*XXXAudioProcessorEditor::XXXAudioProcessorEditor* (this is the first function
in the PluginEditor.cpp file). Add these lines at the end of the function:
    
    // these define the parameters of our slider object
    p1.setSliderStyle(Slider::LinearBarVertical);
    p1.setRange(0.0, 1.0, 0.01);
    p1.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
    p1.setPopupDisplayEnabled(true, true, this);
    p1.setTextValueSuffix(" Delay Time");
    p1.setValue(0.5);
    p1.addListener(this);
    // this function adds the slider to the editor
    addAndMakeVisible(&p1);

    p2.setSliderStyle(Slider::LinearBarVertical);
    p2.setRange(0.0, 1.0, 0.01);
    p2.setTextBoxStyle(Slider::NoTextBox, false, 90, 0);
    p2.setPopupDisplayEnabled(true, true, this);
    p2.setTextValueSuffix(" Delay Feedback");
    p2.setValue(0.5);
    p2.addListener(this);
    addAndMakeVisible(&p2);

Set the size and position of the sliders. Add these lines in 
*XXXAudioProcessorEditor::resized* function:
    
    p1.setBounds(40, 30, 20, getHeight() - 60);
    p2.setBounds(70, 30, 20, getHeight() - 60);

------------------------------------------------------------------------------
Step 10 - Connect the sliders to the audio process

At the end of the file PluginEditor.cpp, create the slider's callback 
function which will pass the values to the audio processing objects:
    
    void XXXAudioProcessorEditor::sliderValueChanged (Slider* slider)
    {
        processor.pyo.value("dtime", p1.getValue());
        processor.pyo.value("feed", p2.getValue());
    }

------------------------------------------------------------------------------

That's it! Compile and Run...

Documentation
-------------

For a complete description of functions used to communicate with the pyo 
embedded processes, see documentation comments in the file PyoClass.cpp.


(c) 2017 - belangeo
