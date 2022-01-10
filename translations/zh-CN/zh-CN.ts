<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="zh_CN">
<context>
    <name>ExtractTargetSelectModel</name>
    <message>
        <location filename="../../src/extracttargetselectmodel.cpp" line="118"/>
        <source>Filename</source>
        <translation>文件名</translation>
    </message>
    <message>
        <location filename="../../src/extracttargetselectmodel.cpp" line="119"/>
        <source>Begin index</source>
        <translation>开始位置</translation>
    </message>
    <message>
        <location filename="../../src/extracttargetselectmodel.cpp" line="120"/>
        <source>Length</source>
        <translation>长度</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="../../src/mainwindow.ui" line="14"/>
        <source>KiraWavTar</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="23"/>
        <source>What to do:</source>
        <translation>要进行的操作：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="30"/>
        <source>Combine WAV files</source>
        <translation>合并波形文件</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="43"/>
        <source>Extract WAV file</source>
        <translation>拆分波形文件</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="81"/>
        <source>Save combined file to:</source>
        <translation>保存合并结果到：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="88"/>
        <source>Target format:</source>
        <translation>目标格式：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="95"/>
        <source>Folder contains WAV files:</source>
        <translation>WAV所在文件夹：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="102"/>
        <source>Make program to scan subfolders (and their subfolders), and combine all WAV files found. The folder structure will be stored in description file, as this will be reconstructed when extract.</source>
        <translation>该选项让程序检查子文件夹（以及他们的子文件夹），并合并这些文件夹中的WAV文件。这些文件夹的结构会写入记录信息中，以备拆分时恢复。</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="105"/>
        <source>scan subfolders</source>
        <translation>也处理子文件夹</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="154"/>
        <source>WAV file to extract:</source>
        <translation>要拆分的WAV文件：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="161"/>
        <source>Choose which ones to extract...</source>
        <translation>选择要被拆分的项...</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="168"/>
        <source>Save results to:</source>
        <translation>保存结果到：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="177"/>
        <source>Use unified format:</source>
        <translation>统一为：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="187"/>
        <source>Same as source when combing</source>
        <translation>和合并时的源文件一致</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="234"/>
        <source>Output format:</source>
        <translation>输出格式：</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="260"/>
        <location filename="../../src/mainwindow.cpp" line="123"/>
        <source>About</source>
        <translation>关于</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="277"/>
        <source>Reset</source>
        <translation>重置</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.ui" line="297"/>
        <source>Run</source>
        <translation>运行</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.cpp" line="78"/>
        <location filename="../../src/mainwindow.cpp" line="94"/>
        <source>Needed paths are empty. Please check your input and try again.</source>
        <translation>没有提供所有需要的路径。请检查输入之后重试。</translation>
    </message>
    <message>
        <location filename="../../src/mainwindow.cpp" line="124"/>
        <source>&lt;h2&gt;KiraWAVTar&lt;/h2&gt;
&lt;p&gt;Copyright 2021 &lt;a href=&quot;https://shine5402.top/about-me&quot;&gt;shine_5402&lt;/a&gt;&lt;/p&gt;
&lt;p&gt;Version %1&lt;/p&gt;
&lt;h3&gt;About&lt;/h3&gt;
&lt;p&gt;A fast and easy-to-use WAV combine/extract tool.&lt;/p&gt;
&lt;h3&gt;License&lt;/h3&gt;
&lt;p&gt; This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.&lt;/p&gt;
&lt;p&gt;This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.&lt;/p&gt;
&lt;p&gt;You should have received a copy of the GNU General Public License
along with this program.  If not, see &lt;a href=&quot;https://www.gnu.org/licenses/&quot;&gt;https://www.gnu.org/licenses/&lt;/a&gt;.&lt;/p&gt;

&lt;h3&gt;3rd party librarays used by this project&lt;/h3&gt;
&lt;ul&gt;
&lt;li&gt;Qt %2, The Qt Company Ltd, under LGPL v3.&lt;/li&gt;
&lt;li&gt;&lt;a href=&quot;https://www.kfrlib.com/&quot;&gt;KFR - Fast, modern C++ DSP framework&lt;/a&gt;, under GNU GPL v2+&lt;/li&gt;
&lt;li&gt;&lt;a href=&quot;https://github.com/shine5402/KiraCommonUtils&quot;&gt;KiraCommmonUtils&lt;/a&gt;, shine_5402, mainly under the Apache License, Version 2.0&lt;/li&gt;
&lt;li&gt;&lt;a href=&quot;https://github.com/mapbox/eternal&quot;&gt;eternal&lt;/a&gt;, mapbox, under ISC License&lt;/li&gt;
&lt;/ul&gt;
</source>
        <translation>&lt;h2&gt;KiraWAVTar&lt;/h2&gt;
&lt;p&gt;版权所有 2021 &lt;a href=&quot;https://shine5402.top/about-me&quot;&gt;shine_5402&lt;/a&gt;&lt;/p&gt;
&lt;p&gt;版本 %1&lt;/p&gt;
&lt;h3&gt;关于&lt;/h3&gt;
&lt;p&gt;快速易用的WAV文件合并/拆分工具。&lt;/p&gt;
&lt;h3&gt;License&lt;/h3&gt;
&lt;h3&gt;许可&lt;/h3&gt;
&lt;p&gt;本程序是自由软件：你可以在遵守由自由软件基金会发布的GNU通用公共许可证版本3（或者更新的版本）的情况下重新分发和/或修改本程序。&lt;/p&gt;
&lt;p&gt;本程序的发布旨在能够派上用场，但是&lt;span style=&quot;font-weight: bold;&quot;&gt;并不对此作出任何担保&lt;/span&gt;；乃至也没有对&lt;span style=&quot;font-weight: bold;&quot;&gt;适销性&lt;/span&gt;或&lt;span style=&quot;font-weight: bold;&quot;&gt;特定用途适用性&lt;/span&gt;的默示担保。参见GNU通用公共许可证来获得更多细节。&lt;/p&gt;
&lt;p&gt;在得到本程序的同时，您应该也收到了一份GNU通用公共许可证的副本。如果没有，请查阅&lt;a href=&quot;https://www.gnu.org/licenses/&quot;&gt;https://www.gnu.org/licenses/&lt;/a&gt;。&lt;/p&gt;

&lt;h3&gt;本程序使用的开源软件库&lt;/h3&gt;
&lt;ul&gt;
&lt;li&gt;Qt %2, The Qt Company Ltd, under LGPL v3.&lt;/li&gt;
&lt;li&gt;&lt;a href=&quot;https://www.kfrlib.com/&quot;&gt;KFR - Fast, modern C++ DSP framework&lt;/a&gt;, under GNU GPL v2+&lt;/li&gt;
&lt;li&gt;&lt;a href=&quot;https://github.com/shine5402/KiraCommonUtils&quot;&gt;KiraCommmonUtils&lt;/a&gt;, shine_5402, mainly under the Apache License, Version 2.0&lt;/li&gt;
&lt;li&gt;&lt;a href=&quot;https://github.com/mapbox/eternal&quot;&gt;eternal&lt;/a&gt;, mapbox, under ISC License&lt;/li&gt;
&lt;/ul&gt;
</translation>
    </message>
</context>
<context>
    <name>WAVCombine</name>
    <message>
        <location filename="../../src/wavcombine.cpp" line="25"/>
        <source>&lt;p class=&apos;critical&apos;&gt;There are not any wav files in the given folder. Please check the path, or if you forget to turn &quot;scan subfolders&quot; on?&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;critical&apos;&gt;没有在所给文件夹中找到任何wav文件。请检查提供的路径，或者忘记勾选“包含子文件夹”了？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="43"/>
        <source>&lt;p class=&apos;critical&apos;&gt;Length of the wav file combined will be too large to save in normal WAVs. Please use W64 WAV instead.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;critical&apos;&gt;合并后的音频数据长度超过了普通WAV文件所能承载的长度，请选择使用W64格式来保存。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="56"/>
        <source>&lt;p class=&apos;warning&apos;&gt;Can not know bit depth from &quot;%1&quot;. Maybe this file id corrupted, or error happend during openning the file.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;我们无法得知“%1”存储的量化类型，可能该文件已损坏，或者文件打开时出现了问题。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="67"/>
        <source>&lt;p class=&apos;warning&apos;&gt;There are %2 channel(s) in &quot;%1&quot;. Channels after No.%3 will be discarded.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;“%1”包含了%2个声道，位于%3号之后的声道数据会被丢弃。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="79"/>
        <source>&lt;p class=&apos;warning&apos;&gt;Sample rate (%2 Hz) of &quot;%1&quot; is larger than target (&quot;%3&quot; Hz).The precision could be lost a bit when processing.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;“%1”的采样率（%2 Hz）比目标（%3 Hz）要大，处理时的重采样会造成一定的精度损失。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="88"/>
        <source>&lt;p class=&apos;warning&apos;&gt;The precision could be lost a bit when processing between &quot;%1&quot; (%2) and target (%3).&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;“%1”的量化类型“%2”在转换到目标“%3”时可能会损失精度。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="99"/>
        <source>&lt;p class=&apos;warning&apos;&gt;The precision could be lost a bit when processing, as we use a bit depth of &quot;%3&quot; in internal processing while source &quot;%1&quot; having &quot;%2&quot;.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;“%1”的量化类型“%2”在处理时可能会损失精度，因为本程序内部统一使用”%3“来进行处理。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="107"/>
        <source>&lt;p class=&apos;warning&apos;&gt;The target file &quot;%1&quot; exists. It will be replaced if proceed, so BACKUP if needed to.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;目标文件“%1”已存在，继续操作的话会覆盖这个文件，如有需要请注意备份。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="122"/>
        <source>Error occurred during reading file &quot;%1&quot;.</source>
        <translation>为读取打开文件%1时出现错误。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="126"/>
        <source>Cannot know sample type of file &quot;%1&quot;.</source>
        <translation>无法得知文件%1的采样类型。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="197"/>
        <source>No data to combine.</source>
        <translation>没有可被合并的数据。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="217"/>
        <source>Error occurred during writing file &quot;%1&quot;.</source>
        <translation>为写入打开文件%1时出现错误。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombine.cpp" line="224"/>
        <source>File &quot;%1&quot; can not being fully written. %2 bytes has been written into file &quot;%1&quot;, which is expected to be %3.</source>
        <translation>文件%1写入的字节数（%2）和预期的（%3）不一致（即没有完全写入完成）。</translation>
    </message>
</context>
<context>
    <name>WAVCombineDialog</name>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="43"/>
        <source>Some preparing work...</source>
        <translation>一些准备工作……</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="67"/>
        <source>Critical error found. Can not continue.</source>
        <translation>发现了一些严重问题，操作无法继续。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="75"/>
        <source>Some problems have been found but process can continue. Should we proceed?</source>
        <translation>发现了一些问题，不过操作仍然可以继续。请问要继续吗？</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="88"/>
        <source>Reading WAV files and combining them...</source>
        <translation>读取波形文件及拼接……</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="107"/>
        <source>Writing combined file...</source>
        <translation>写入合并后的文件……</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="126"/>
        <source>Done</source>
        <translation>完成</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="134"/>
        <source>Wav files has been combined.</source>
        <translation>合并操作已经完成。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="135"/>
        <source>Combined file has been stored at &quot;%1&quot;.Please do not change the time when you edit it, and do not delete or modify &quot;.kirawavtar-desc.json&quot; file sharing the same name with the WAV.</source>
        <translation>合并后的波形文件已经存储至%1。请注意在处理时不要修改波形文件内的时值，也不要删除和修改同名的“.kirawavtar-desc.json”描述文件。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="146"/>
        <source>Error occurred when writing combined WAV.</source>
        <translation>写入合并结果文件时出现问题。</translation>
    </message>
    <message>
        <location filename="../../src/wavcombinedialog.cpp" line="147"/>
        <source>Please check potential causes and try again.</source>
        <translation>请排查可能问题后再试。</translation>
    </message>
</context>
<context>
    <name>WAVExtract</name>
    <message>
        <location filename="../../src/wavextract.cpp" line="26"/>
        <source>&lt;p class=&apos;critical&apos;&gt;The file given or its description file don&apos;t exists.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;critical&apos;&gt;提供的波形文件或者其对应的描述文件不存在。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="31"/>
        <source>&lt;p class=&apos;critical&apos;&gt;The description can not be opened.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;critical&apos;&gt;无法打开描述文件。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="38"/>
        <source>&lt;p class=&apos;critical&apos;&gt;The desctiption file has a incompatible version.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;critical&apos;&gt;描述文件的版本和我们当前使用的不兼容。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="43"/>
        <source>&lt;p class=&apos;critical&apos;&gt;The wav file can not be opened.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;critical&apos;&gt;无法打开波形文件。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="48"/>
        <source>&lt;p class=&apos;warning&apos;&gt;The wav file has a different length (%1 samples) than expected (%2 samples). Sample rate changed or time has been changed?&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;波形文件的长度“%1”和描述文件告知的“%2”不一致。修改了采样率还是进行了时值修改？&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="52"/>
        <source>&lt;p class=&apos;warning&apos;&gt;Sample rate (%1) of the given file is not same as it supposed to be (%2) in description file. We will resample it.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;波形文件的采样率“%1 Hz”和描述文件告知“%2 Hz”的不一致。我们会进行重采样。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="57"/>
        <source>&lt;p class=&apos;warning&apos;&gt;Channel count (%1) of the given file is not same as it supposed to be (%2) in description file.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;波形文件的声道数“%1”与预期的“%2”不符。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="61"/>
        <source>&lt;p class=&apos;warning&apos;&gt;It seems like that target folder is not empty. All files have name conflicts will be replaced, so BACKUP if needed to.&lt;/p&gt;</source>
        <translation>&lt;p class=&apos;warning&apos;&gt;看起来目标文件夹不为空。在拆分过程中所有会产生名称冲突的文件都会被替换，如果有必要的话请做好备份。&lt;/p&gt;</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="71"/>
        <source>Error occurred when opening &quot;%1&quot;.</source>
        <translation>打开文件%1时出现错误。</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="75"/>
        <source>There have no data in file &quot;%1&quot;.</source>
        <translation>文件%1时中没有数据。</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="161"/>
        <source>Error occurred when writing into &quot;%1&quot;.</source>
        <translation>为写入打开文件%1时出现错误。</translation>
    </message>
    <message>
        <location filename="../../src/wavextract.cpp" line="169"/>
        <source>File &quot;%1&quot; can not being fully written. %2 bytes has been written into file &quot;%1&quot;, which is expected to be %3.</source>
        <translation>文件%1写入的字节数（%2）和预期的（%3）不一致（即没有完全写入完成）。</translation>
    </message>
</context>
<context>
    <name>WAVExtractDialog</name>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="47"/>
        <source>Some preparing work...</source>
        <translation>一些准备工作……</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="71"/>
        <source>Critical error found. Can not continue.</source>
        <translation>发现了一些严重问题，操作无法继续。</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="79"/>
        <source>Some problems have been found but process can continue. Should we proceed?</source>
        <translation>发现了一些问题，不过操作仍然可以继续。请问要继续吗？</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="92"/>
        <source>Reading source WAV file...</source>
        <translation>读取源波形文件……</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="105"/>
        <source>Writing extracted file...</source>
        <translation>拆分波形文件并写入……</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="130"/>
        <source>Choose which ones to extract.</source>
        <translation>请选择要被拆分的项。</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="141"/>
        <source>Select all</source>
        <translation>选择全部</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="142"/>
        <source>Unselect all</source>
        <translation>取消选择全部</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="143"/>
        <source>Inverse</source>
        <translation>选择反向</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="179"/>
        <source>Done</source>
        <translation>完成</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="188"/>
        <source>The wav file has been extracted.</source>
        <translation>拆分操作已经完成。</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="189"/>
        <source>Extracted wav files has been stored at &quot;%1&quot;.Original folder structure has been kept too.</source>
        <translation>拆分后的波形文件已经存储至%1，原先的文件夹结构也已经被还原（如果有）。</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="192"/>
        <source>Delete source file</source>
        <translation>删除源文件</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="197"/>
        <location filename="../../src/wavextractdialog.cpp" line="199"/>
        <source>Can not delete %1</source>
        <translation>无法删除%1</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="201"/>
        <source>Source files have been deleted successfully.</source>
        <translation>成功删除了源文件。</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="213"/>
        <source>Error occurred when extracting.</source>
        <translation>拆分时出现错误。</translation>
    </message>
    <message>
        <location filename="../../src/wavextractdialog.cpp" line="225"/>
        <source>Should retry extracting these?</source>
        <translation>要重试拆分这些文件吗？</translation>
    </message>
</context>
<context>
    <name>WAVFormatChooserWidget</name>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="29"/>
        <source>Sample rate</source>
        <translation>采样率</translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="43"/>
        <source>11025</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="48"/>
        <source>22050</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="53"/>
        <source>44100</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="58"/>
        <source>48000</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="63"/>
        <source>88200</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="68"/>
        <source>96000</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="73"/>
        <source>176400</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="78"/>
        <source>192000</source>
        <translation></translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="86"/>
        <source>Bit depth</source>
        <translation>位深度</translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="96"/>
        <source>Channels</source>
        <translation>声道数</translation>
    </message>
    <message>
        <location filename="../../src/wavformatchooserwidget.ui" line="110"/>
        <source>Use W64 format</source>
        <translation>使用W64格式</translation>
    </message>
</context>
<context>
    <name>WAVTarUtils</name>
    <message>
        <location filename="../../src/wavtar_utils.h" line="23"/>
        <source>Unknown error occurred.</source>
        <translation>发生了未知错误。</translation>
    </message>
</context>
</TS>
