"""A small, editable starter catalogue. Each pack includes three question types."""
import json

SUBJECTS = ['Mathematics', 'Science', 'English', 'Computing']
PACKS = {
 'Mathematics': [
  ('Number sense', [('mcq','What is 3/4 expressed as a decimal?',['0.25','0.5','0.75','1.25'],'0.75','Divide 3 by 4 to get 0.75.'),('true_false','Every square is also a rectangle.',['True','False'],'True','A square has four right angles, so it meets the definition of a rectangle.'),('short_answer','What is the area of a rectangle with length 8 and width 5? Enter the number.',[],'40','Area is length × width, so 8 × 5 = 40.')]),
  ('Algebra essentials', [('mcq','Solve 3x + 6 = 21.',['3','5','7','9'],'5','Subtract 6, then divide by 3.'),('true_false','The gradient of y = 2x + 3 is 3.',['True','False'],'False','The coefficient of x is the gradient, which is 2.'),('short_answer','What is the positive square root of 144?',[],'12','12 × 12 = 144.')]),
  ('Functions & calculus', [('mcq','What is the derivative of x³?',['x²','3x²','3x','x³/3'],'3x²','The power rule gives d(xⁿ)/dx = nxⁿ⁻¹.'),('true_false','The sine function has a period of 2π radians.',['True','False'],'True','Sine repeats every full rotation of 2π.'),('short_answer','Evaluate log10(1000). Enter the number.',[],'3','10 raised to the power 3 equals 1000.')])],
 'Science': [
  ('The world around us', [('mcq','Which organ pumps blood around the body?',['Brain','Heart','Lung','Kidney'],'Heart','The heart pumps blood through the circulatory system.'),('true_false','Plants use sunlight in photosynthesis.',['True','False'],'True','Light supplies the energy for photosynthesis.'),('short_answer','What is the chemical formula for water?',[],'H2O','Each water molecule has two hydrogen atoms and one oxygen atom.')]),
  ('Matter & energy', [('mcq','What is the SI unit of force?',['Watt','Joule','Newton','Pascal'],'Newton','Force is measured in newtons.'),('true_false','An atom is electrically neutral when it has equal numbers of protons and electrons.',['True','False'],'True','Positive and negative charges balance.'),('short_answer','Which gas do plants absorb during photosynthesis?',[],'carbon dioxide','Plants absorb carbon dioxide to build glucose.')]),
  ('Physics & chemistry', [('mcq','Which quantity is conserved in an isolated system?',['Velocity','Momentum','Acceleration','Temperature'],'Momentum','Total momentum is conserved when no external force acts.'),('true_false','A catalyst changes the equilibrium constant of a reaction.',['True','False'],'False','A catalyst speeds both forward and reverse reactions without changing equilibrium.'),('short_answer','What is the pH of a neutral solution at 25°C?',[],'7','At 25°C, a neutral solution has pH 7.')])],
 'English': [
  ('Words & sentences', [('mcq','Which word is an adjective?',['Quickly','Beautiful','Run','They'],'Beautiful','An adjective describes a noun.'),('true_false','A complete sentence usually includes a subject and a verb.',['True','False'],'True','The subject and verb form the core of a sentence.'),('short_answer','Write the plural of child.',[],'children','Child has the irregular plural children.')]),
  ('Grammar in context', [('mcq','Choose the correct sentence.',['She have finished.','She has finished.','She having finished.','She finish yesterday.'],'She has finished.','Use has with a singular third-person subject in the present perfect.'),('true_false','A metaphor compares things using like or as.',['True','False'],'False','A simile uses like or as; a metaphor makes a direct comparison.'),('short_answer','Write the past participle of write.',[],'written','The verb forms are write, wrote, written.')]),
  ('Language & literature', [('mcq','Which device uses an intentional exaggeration?',['Irony','Hyperbole','Alliteration','Onomatopoeia'],'Hyperbole','Hyperbole exaggerates for emphasis.'),('true_false','An unreliable narrator may present a distorted account of events.',['True','False'],'True','The reader must assess the narrator’s credibility.'),('short_answer','What is the term for the main character of a story?',[],'protagonist','The protagonist is the central character.')])],
 'Computing': [
  ('Digital foundations', [('mcq','Which device is used to input text?',['Monitor','Keyboard','Speaker','Projector'],'Keyboard','A keyboard sends typed input to a computer.'),('true_false','A strong password should be shared with classmates.',['True','False'],'False','Passwords should be kept private.'),('short_answer','How many bits are in one byte?',[],'8','One byte consists of eight bits.')]),
  ('Code & logic', [('mcq','Which structure repeats instructions?',['Loop','Variable','Comment','String'],'Loop','A loop repeats a block of instructions.'),('true_false','Binary numbers use only the digits 0 and 1.',['True','False'],'True','Binary is a base-two number system.'),('short_answer','What is the decimal value of binary 1010?',[],'10','8 + 2 = 10.')]),
  ('Algorithms & data', [('mcq','What is the time complexity of binary search on a sorted array?',['O(n²)','O(n)','O(log n)','O(1)'],'O(log n)','Each step halves the search space.'),('true_false','A stack follows a last-in, first-out order.',['True','False'],'True','The most recently pushed item is popped first.'),('short_answer','What does the S in HTTPS stand for?',[],'secure','HTTPS means Hypertext Transfer Protocol Secure.')])]
}

def seed_catalog(db):
    if db.execute('SELECT COUNT(*) FROM quizzes').fetchone()[0]:
        return
    for grade in range(6, 13):
        band = 0 if grade <= 8 else 1 if grade <= 10 else 2
        for subject in SUBJECTS:
            title, questions = PACKS[subject][band]
            payload = [dict(type=t, prompt=p, options=o, answer=a, explanation=e)
                       for t, p, o, a, e in questions]
            db.execute('INSERT INTO quizzes(title,subject,grade,description,questions) VALUES(?,?,?,?,?)',
                       (title, subject, grade, f'Build confidence in {subject.lower()} with a focused mixed-format practice.', json.dumps(payload)))
    db.commit()
