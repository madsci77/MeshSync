VERSION 5.00
Begin VB.Form frmMesh 
   Caption         =   "Mesh Network Simulator"
   ClientHeight    =   10395
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   14835
   LinkTopic       =   "Form1"
   ScaleHeight     =   10395
   ScaleWidth      =   14835
   StartUpPosition =   3  'Windows Default
   Begin VB.Timer Timer3 
      Interval        =   1000
      Left            =   120
      Top             =   1320
   End
   Begin VB.CheckBox chkRange 
      Caption         =   "Range"
      Height          =   495
      Left            =   14160
      Style           =   1  'Graphical
      TabIndex        =   1
      Top             =   9120
      Width           =   615
   End
   Begin VB.CheckBox chkProgram 
      Caption         =   "Mode"
      Height          =   495
      Left            =   14160
      Style           =   1  'Graphical
      TabIndex        =   0
      Top             =   9720
      Width           =   615
   End
   Begin VB.Timer Timer2 
      Enabled         =   0   'False
      Interval        =   20
      Left            =   120
      Top             =   720
   End
   Begin VB.Timer Timer1 
      Enabled         =   0   'False
      Interval        =   20
      Left            =   120
      Top             =   120
   End
   Begin VB.Shape shpTx 
      Height          =   2415
      Left            =   3120
      Shape           =   3  'Circle
      Top             =   3960
      Visible         =   0   'False
      Width           =   2535
   End
End
Attribute VB_Name = "frmMesh"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
Option Explicit

Private Type NodeType
    Shape As Object
    TopLabel As Object
    BottomLabel As Object
    LeftLabel As Object
    RightLabel As Object
    Range As Integer
    ID As Integer
    Group As Integer
    GroupTimeout As Integer
    Sequence As Integer
    RelativePosition As Integer
    NodesInGroup As Integer
    DirectHeard() As Boolean
    CurrentMembers() As Boolean
End Type


Private Type MessageType
    SourceID As Integer
    GroupID As Integer
    Sequence As Integer
    DirectHeard() As Boolean
    CurrentMembers() As Boolean
End Type

Dim Node() As NodeType

Dim MaxNodes As Integer
Dim DragNode As Integer             ' Node currently being moved by mouse
Dim NodeCount As Integer            ' Total number of nodes in system
Dim TimeSlot As Integer             ' Transmit cycle time slot
Dim TimeInc As Integer
Dim NetTimeout As Integer
Dim HistCount As Integer
Dim Cycle As Integer

Dim NodeSize, MinRange, MaxRange, TickCount, ProgramSequence As Integer

Private Sub Form_Load()
    Dim n, m As Integer
    
    Timer1.Interval = 6
    Timer2.Interval = Timer1.Interval - 1
    
    MaxNodes = 50
    NodeSize = 300
    MinRange = 1500
    MaxRange = 1500
    NetTimeout = 6
    HistCount = 5
    ProgramSequence = 0

    Randomize
        
    DragNode = 0
    NodeCount = 25
    TimeSlot = 1
    TimeInc = 1
    Cycle = 0
    
    ReDim Node(MaxNodes)
    ' Create and draw shapes for all nodes
    For n = 1 To NodeCount
        ReDim Node(n).DirectHeard(HistCount, MaxNodes)
        ReDim Node(n).CurrentMembers(HistCount, MaxNodes)
        Set Node(n).Shape = Controls.Add("vb.Shape", "nodeshape" & CStr(n))
        Set Node(n).TopLabel = Controls.Add("vb.Label", "nodeTopLabel" & CStr(n))
        Set Node(n).BottomLabel = Controls.Add("vb.Label", "nodeBottomLabel" & CStr(n))
        Set Node(n).LeftLabel = Controls.Add("vb.Label", "nodeLeftLabel" & CStr(n))
        Set Node(n).RightLabel = Controls.Add("vb.Label", "nodeRightLabel" & CStr(n))
        ' Set random TX range for node
        Node(n).Range = MinRange + (Rnd(1) * (MaxRange - MinRange))
        Node(n).Shape.Left = NodeSize / 2 + (Rnd(1) * (frmMesh.Width - NodeSize * 2))
        Node(n).Shape.Top = NodeSize / 2 + (Rnd(1) * (frmMesh.Height - NodeSize * 2))
        Node(n).ID = n
        Node(n).Group = n
        Node(n).RelativePosition = 1
        Node(n).NodesInGroup = 1
        Node(n).Sequence = 0
        If chkProgram.Value = Unchecked Then Node(n).Shape.FillColor = vbRed
        DrawNode Node(n)
    Next n
    
    Timer1.Enabled = True
End Sub


Private Sub Timer1_Timer()
    ' Network tick
    
    Dim Msg As MessageType
    Dim n, c, tx, h, m As Integer
    Dim IsMember As Boolean
    
    ReDim Msg.DirectHeard(MaxNodes)
    ReDim Msg.CurrentMembers(MaxNodes)
    
    ' Current node transmits - compose message
    tx = TimeSlot
    
    Msg.SourceID = tx
    Msg.GroupID = Node(tx).Group
    Msg.Sequence = Node(tx).Sequence
    
    ' Be sure to include ourself if we're a group leader in the known list
    If Node(tx).ID = Node(tx).Group Then Node(tx).CurrentMembers(1, Node(tx).ID) = True
    
    For c = 1 To NodeCount
        ' Just send our known lists
        Msg.DirectHeard(c) = Node(tx).DirectHeard(1, c)
        Msg.CurrentMembers(c) = Node(tx).CurrentMembers(1, c)
    Next c
    
    SendMessage Node(tx), Msg
           
    ' Advance to next time slot
    
    TimeSlot = TimeSlot + TimeInc
    
    ' When we get to the highest time slot, start counting backwards
    If TimeInc = 1 And TimeSlot >= NodeCount Then
        TimeInc = -1
    End If
    
    If TimeInc = -1 And TimeSlot = 0 Then
        Cycle = Cycle + 1
        TimeInc = 1
        TimeSlot = 1
    End If
    
   
    If Cycle = 3 Then
        Cycle = 0
'        System_Cycle
    End If
    
    TickCount = TickCount + 1
    If TickCount = 5 Then
        ProgramTick
        TickCount = 0
    End If
    
End Sub

Private Sub System_Cycle()
    Dim n, c, tx, h, m As Integer
    Dim pos As Integer
    ' End of net cycle events
    Debug.Print "- End of Cycle -"
    
    ' Cycle through all nodes
    For n = 1 To NodeCount
        ' Update RX timeout timer
        If Node(n).GroupTimeout > 0 Then
            Node(n).GroupTimeout = Node(n).GroupTimeout - 1
            If Node(n).GroupTimeout = 0 Then
                ' Node's RX timer has expired, fall back to standalone mode
                If chkProgram.Value = Unchecked Then Node(n).Shape.FillColor = vbRed
                Node(n).Group = Node(n).ID
                DrawNode Node(n)
            End If
        End If
        
        ' See if we lost our group leader (if we're not leader)
        If Node(n).ID <> Node(n).Group And OrHistory(Node(n).CurrentMembers, Node(n).Group) = False Then
            Debug.Print "Node " & Node(n).ID & "(" & Node(n).Group & ") Lost group leader"
            ' Default to our own group
            Node(n).Group = Node(n).ID
            ' Search for lower-numbered nodes in our known list
'            For m = Node(n).ID To 1 Step -1
 '               If OrHistory(Node(n).CurrentMembers, m) = True Then Node(n).Group = m
  '          Next m
            Debug.Print "   Joined group " & Node(n).Group
        End If
        
        For c = 1 To NodeCount
            ' Cycle history lists
            For h = HistCount To 2 Step -1
                Node(n).CurrentMembers(h, c) = Node(n).CurrentMembers(h - 1, c)
            Next h
            ' Non-group leaders erase current member list
            If Node(n).ID <> Node(n).Group Then
                Node(n).CurrentMembers(1, c) = False
            ' Group leaders aggregate all heard stations in their history
            Else
                Node(n).CurrentMembers(1, c) = False
                For h = 2 To HistCount
                    If Node(n).DirectHeard(h, c) = True Then Node(n).CurrentMembers(1, c) = True
                Next h
                Node(n).CurrentMembers(1, n) = True
                ' Cycle direct heard history
                For h = HistCount To 2 Step -1
                    Node(n).DirectHeard(h, c) = Node(n).DirectHeard(h - 1, c)
                Next h
            End If
            Node(n).DirectHeard(1, c) = False
        Next c
        
        ' Find our relative position in this group
        Node(n).RelativePosition = 0
        Node(n).NodesInGroup = 0
        For c = 1 To NodeCount
            pos = c
            If OrHistory(Node(n).CurrentMembers, pos) = True Then
                ' Node is a member of our group
                Node(n).NodesInGroup = Node(n).NodesInGroup + 1
                If c <= n Then Node(n).RelativePosition = Node(n).RelativePosition + 1 ' Count those before us
            End If
        Next c
        DrawNode Node(n)
        
    Next n

End Sub


Private Sub ProgramTick()
    Dim c As Integer
'    ProgramSequence = ProgramSequence + 1
    If chkProgram.Value = Checked Then
        For c = 1 To NodeCount
        
            ' Lighting program run for all nodes
            
            If Node(c).Sequence = Node(c).RelativePosition Or _
              Node(c).Sequence - Node(c).NodesInGroup = Node(c).NodesInGroup - Node(c).RelativePosition Then
                Node(c).Shape.FillColor = vbBlue
            Else
                Node(c).Shape.FillColor = vbWhite
            End If
            
            Node(c).Sequence = Node(c).Sequence + 1
            If Node(c).Sequence >= Node(c).NodesInGroup * 2 Then Node(c).Sequence = 2
            
        Next c
    End If
'    If ProgramSequence = NodeCount Then ProgramSequence = 0
End Sub

Private Function OrHistory(hist() As Boolean, n As Integer)
    Dim h As Integer
    OrHistory = False
    For h = 1 To HistCount
        If hist(h, n) = True Then OrHistory = True
    Next h
End Function

Private Function ArrayToString(arr() As Boolean)
    Dim s As String
    Dim n As Integer
    For n = 1 To NodeCount
        If arr(1, n) = True Then
            s = s & "*"
        Else
            s = s & "."
        End If
    Next n
    ArrayToString = s
End Function

Private Sub Timer2_Timer()
    ' Turn off TX range indicator circle
    Timer2.Enabled = False
    shpTx.Visible = False
End Sub

Private Function GetDistance(obj1 As Shape, obj2 As Shape) As Double
    ' Calculate distance between object centers using Pythagorean theorem
    Dim a, b As Double
    a = (obj1.Left + obj1.Width / 2) - (obj2.Left + obj2.Width / 2)
    b = (obj1.Top + obj1.Height / 2) - (obj2.Top + obj2.Height / 2)
    GetDistance = Sqr(a * a + b * b)
End Function

' Sends a given message from the specified node, will be delivered to all nodes in range
Private Function SendMessage(Origin As NodeType, Message As MessageType)
    Dim c
    
    Debug.Print "Node " & Origin.ID & "(" & Origin.Group & ") transmitted"
    
    Debug.Print ArrayToString(Origin.CurrentMembers) & " " & ArrayToString(Origin.DirectHeard)
    
    ' Draw range circle around node
    If chkRange = Checked Then
        shpTx.Width = Origin.Range * 2
        shpTx.Height = Origin.Range * 2
        shpTx.Left = Origin.Shape.Left + Origin.Shape.Width / 2 - shpTx.Width / 2
        shpTx.Top = Origin.Shape.Top + Origin.Shape.Height / 2 - shpTx.Height / 2
        shpTx.Visible = True
    End If
    
    ' Set timer to erase range circle
    Timer2.Enabled = True
    
    ' Determine nodes in range
    For c = 1 To NodeCount
        ' Exclude originating station (it doesn't hear itself)
        If c <> Origin.ID Then
            If GetDistance(Node(c).Shape, Origin.Shape) <= Origin.Range Then
                ' Node is in range
                ReceiveMessage Node(c), Message
            End If
        End If
    Next c
End Function

' Called for each node that receives a message
Private Function ReceiveMessage(ByRef Recipient As NodeType, Message As MessageType)
    Dim n, c, h As Integer
    Dim IsMember As Boolean
    
    Debug.Print "  Node " & Recipient.ID & "(" & Recipient.Group & ") received"
    IsMember = False
    If chkProgram.Value = Unchecked Then Recipient.Shape.FillColor = vbGreen       ' Set color to indicate RX
      
    ' If received group ID is our own group ID, update timeout
    If Recipient.Group = Message.GroupID Then
        Recipient.GroupTimeout = NetTimeout
    
        If Message.SourceID < Recipient.ID Then Recipient.Sequence = Message.Sequence
     
        ' Record that we've heard this transmitting station
        Recipient.DirectHeard(1, Message.SourceID) = True

        ' Record official member list, update this cycle's heard members
        For n = 1 To NodeCount
            ' Add to current heard list everything that others have heard this cycle
            If Message.DirectHeard(n) = True Then Recipient.DirectHeard(1, n) = True
            ' If we're not group leader, accept member list
            If Recipient.Group <> Recipient.ID Then
                If Message.CurrentMembers(n) = True Then Recipient.CurrentMembers(1, n) = True
            End If
        Next n
        Debug.Print ArrayToString(Recipient.CurrentMembers) & " " & ArrayToString(Recipient.DirectHeard)
    End If
    
      ' If received group ID is lower, accept it as our new group ID
    If Recipient.Group > Message.GroupID Then
        Recipient.Group = Message.GroupID
        Debug.Print "  Joined group " & Message.GroupID
        ' Throw out our known station list and accept list from new group
        For c = 1 To NodeCount
            Recipient.CurrentMembers(1, c) = Message.CurrentMembers(c)
            Recipient.DirectHeard(1, c) = Message.DirectHeard(c)
        Next c
        ' Clear history for previous group
        For h = 1 To HistCount
            For c = 1 To NodeCount
                Recipient.CurrentMembers(h, c) = False
                Recipient.DirectHeard(h, c) = False
            Next c
        Next h
    End If

    ' Check last cycle's official group member list to see if we're part of the group
    If Recipient.CurrentMembers(2, Recipient.ID) = True Then IsMember = True
    If IsMember = False And chkProgram.Value = Unchecked Then Recipient.Shape.FillColor = vbYellow
    
    DrawNode Recipient

End Function


Private Sub DrawNode(dnode As NodeType)
    dnode.Shape.Shape = 3
    dnode.Shape.FillStyle = vbSolid
    dnode.Shape.Height = NodeSize
    dnode.Shape.Width = NodeSize
    dnode.Shape.DrawMode = 13
    dnode.Shape.Visible = True
    
    dnode.TopLabel.Caption = CStr(dnode.ID)
    dnode.TopLabel.AutoSize = True
    dnode.TopLabel.ForeColor = vbBlack
    dnode.TopLabel.Top = dnode.Shape.Top - dnode.TopLabel.Height
    dnode.TopLabel.Left = dnode.Shape.Left + dnode.Shape.Width / 2 - dnode.TopLabel.Width / 2
    dnode.TopLabel.Visible = True

    dnode.BottomLabel.Caption = CStr(dnode.Group)
    dnode.BottomLabel.AutoSize = True
    dnode.BottomLabel.ForeColor = vbBlack
    dnode.BottomLabel.Top = dnode.Shape.Top + dnode.Shape.Height
    dnode.BottomLabel.Left = dnode.Shape.Left + dnode.Shape.Width / 2 - dnode.BottomLabel.Width / 2
    dnode.BottomLabel.Visible = True

    dnode.LeftLabel.Caption = CStr(dnode.NodesInGroup)
    dnode.LeftLabel.AutoSize = True
    dnode.LeftLabel.ForeColor = vbBlack
    dnode.LeftLabel.Top = dnode.Shape.Top + dnode.Shape.Height / 2 - dnode.LeftLabel.Height / 2
    dnode.LeftLabel.Left = dnode.Shape.Left - dnode.LeftLabel.Width
    dnode.LeftLabel.Visible = True
    
    dnode.RightLabel.Caption = CStr(dnode.RelativePosition)
    dnode.RightLabel.AutoSize = True
    dnode.RightLabel.ForeColor = vbBlack
    dnode.RightLabel.Top = dnode.Shape.Top + dnode.Shape.Height / 2 - dnode.LeftLabel.Height / 2
    dnode.RightLabel.Left = dnode.Shape.Left + dnode.Shape.Width
    dnode.RightLabel.Visible = True

End Sub

Private Sub Form_MouseDown(Button As Integer, Shift As Integer, X As Single, Y As Single)
    Dim n, clicked As Integer
    ' Identify node, if any, that mouse has been clicked on
    For n = 1 To NodeCount
        If ((X >= Node(n).Shape.Left And X <= Node(n).Shape.Left + Node(n).Shape.Width) And _
            (Y >= Node(n).Shape.Top And Y <= Node(n).Shape.Top + Node(n).Shape.Height)) Then
            clicked = n
        End If
    Next n
    If Button = 1 Then
        DragNode = clicked
    Else
        MsgBox "Node " & Node(clicked).ID & vbCrLf & _
            "Seq " & Node(clicked).Sequence & vbCrLf & _
            "NodesInGroup " & Node(clicked).NodesInGroup
    End If
End Sub

Private Sub Form_MouseMove(Button As Integer, Shift As Integer, X As Single, Y As Single)
    ' If we've clicked on a node, drag it with the mouse
    If DragNode <> 0 Then
        Node(DragNode).Shape.Left = X - (Node(DragNode).Shape.Width / 2)
        Node(DragNode).Shape.Top = Y - (Node(DragNode).Shape.Height / 2)
        DrawNode Node(DragNode)
    End If
End Sub

Private Sub Form_MouseUp(Button As Integer, Shift As Integer, X As Single, Y As Single)
    ' Mouse has been released, we're no longer dragging a node
    DragNode = 0
End Sub

Private Sub Timer3_Timer()
    System_Cycle
End Sub
