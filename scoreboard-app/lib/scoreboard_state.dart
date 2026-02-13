enum ClockMode {
  running,
  stopped,
  clock,
  intermission,
}

class Penalty {
  final int secondsRemaining;
  final int playerNumber;

  Penalty({required this.secondsRemaining, required this.playerNumber});

  factory Penalty.fromJson(Map<String, dynamic> json) {
    return Penalty(
      secondsRemaining: json['secondsRemaining'] as int,
      playerNumber: json['playerNumber'] as int,
    );
  }
}

class ScoreboardState {
  final int homeScore;
  final int awayScore;
  final int timeMinutes;
  final int timeSeconds;
  final int homeShots;
  final int awayShots;
  final List<Penalty> homePenalties;
  final List<Penalty> awayPenalties;
  final int currentPeriod;
  final String homeTeamName;
  final String awayTeamName;
  final ClockMode clockMode;

  ScoreboardState({
    required this.homeScore,
    required this.awayScore,
    required this.timeMinutes,
    required this.timeSeconds,
    required this.homeShots,
    required this.awayShots,
    required this.homePenalties,
    required this.awayPenalties,
    required this.currentPeriod,
    required this.homeTeamName,
    required this.awayTeamName,
    required this.clockMode,
  });

  factory ScoreboardState.fromJson(Map<String, dynamic> json) {
    return ScoreboardState(
      homeScore: json['homeScore'] as int,
      awayScore: json['awayScore'] as int,
      timeMinutes: json['timeMinutes'] as int,
      timeSeconds: json['timeSeconds'] as int,
      homeShots: json['homeShots'] as int,
      awayShots: json['awayShots'] as int,
      homePenalties: (json['homePenalties'] as List)
          .map((e) => Penalty.fromJson(e as Map<String, dynamic>))
          .toList(),
      awayPenalties: (json['awayPenalties'] as List)
          .map((e) => Penalty.fromJson(e as Map<String, dynamic>))
          .toList(),
      currentPeriod: json['currentPeriod'] as int,
      homeTeamName: json['homeTeamName'] as String,
      awayTeamName: json['awayTeamName'] as String,
      clockMode: _parseClockMode(json['clockMode'] as String),
    );
  }

  static ClockMode _parseClockMode(String mode) {
    switch (mode) {
      case 'Running':
        return ClockMode.running;
      case 'Clock':
        return ClockMode.clock;
      case 'Intermission':
        return ClockMode.intermission;
      default:
        return ClockMode.stopped;
    }
  }
}
